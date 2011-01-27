/*  BikechainJS (engine)
    v0.0.1.10 (c) Kyle Simpson
    MIT License
*/

#include <v8.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string>
#include <time.h>
#include <cstdlib>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef _WIN32
   #include <sys/time.h>
   #include <windows.h>
   #include <winevt.h>
#else
   #include <syslog.h>
#endif

#include <engine.h>

int content_type_needed = 0;
bool backchannel_ready = false;

FILE *file_log_file_handle;
FILE *file_log_error_file_handle;
std::string bikechain_root_path_directory;
v8::Handle<v8::Context> backchannel_context;
v8::Handle<v8::Context> engine_context;
v8::Handle<v8::Value> tmpval;
v8::Handle<v8::Function> jsonstringify_func;
v8::Handle<v8::Function> getcfg_func;
v8::Handle<v8::Function> setcfg_func;

// From: http://www.gnu.org/s/libc/manual/html_node/Working-Directory.html
char* gnu_getcwd() {
	size_t size = 100;
	while (1) {
		char *buffer = (char*)malloc(size);
		if (getcwd (buffer, size) == buffer) {
			return buffer;
		}
		free(buffer);
		if (errno != ERANGE) {
			return 0;
		}
		size *= 2;
	}
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

// converts a v8::Value handle to a std::string
std::string ToStdString(v8::Handle<v8::Value> value) {
	return std::string(ToCString(v8::String::Utf8Value(value)));
}

v8::Handle<v8::Value> ioRead(const v8::Arguments& args) {
	v8::String::Utf8Value nonblocking(args[0]);
	return ReadIO((strcmp(ToCString(nonblocking),"true") == 0));
}

v8::Handle<v8::Value> ioWrite(const v8::Arguments& args) {
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		v8::String::Utf8Value str(args[i]);
		WriteIO(ToCString(str));
	}
	return v8::Undefined();
}

v8::Handle<v8::Value> ioFlush(const v8::Arguments& args) {
	FlushIO();
	return v8::Undefined();
}

v8::Handle<v8::Value> fsRead(const v8::Arguments& args) {
	if (args.Length() != 1) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL) {
		return v8::ThrowException(v8::String::New("Error loading file"));
	}
	v8::Handle<v8::Value> source = ReadFile(ToCString(file));
	return source;
}

v8::Handle<v8::Value> fsWrite(const v8::Arguments& args) {
	if (args.Length() != 2) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	v8::String::Utf8Value file(args[0]);
	v8::String::Utf8Value content(args[1]);
	if (*file == NULL || *content == NULL) {
		return v8::ThrowException(v8::String::New("Error writing file"));
	}
	
	WriteFile(ToCString(file),ToCString(content),strlen(ToCString(content)));
	
	return v8::Undefined();
}

v8::Handle<v8::Value> sysExec(const v8::Arguments& args) {
	if (args.Length() < 1) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	v8::String::Utf8Value command(args[0]);
	if (*command == NULL) {
		return v8::ThrowException(v8::String::New("Command parsing broken"));
	}
	
	return executeProcess(args);
}

v8::Handle<v8::Value> sysExit(const v8::Arguments& args) {
	if (args.Length() != 1) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	
	if (content_type_needed) {
		printf("Content-type: text/plain\n\n");
	}
	
	v8::String::Utf8Value exitStatus(args[0]);
	int status = atoi(ToCString(exitStatus));
	v8::V8::Dispose();
	fclose(file_log_file_handle);
	fclose(file_log_error_file_handle);
	exit(status);
}

v8::Handle<v8::Value> sysConsole(const v8::Arguments& args) {
	if (args.Length() != 2) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	
	std::string console_type = ToStdString(args[0]);
	std::string console_msg = ToStdString(args[1]);
	
	if (console_type == "notice") return v8::Boolean::New(log_notice(console_msg));
	else if (console_type == "log") return v8::Boolean::New(log_console(console_msg));
	else if (console_type == "warn") return v8::Boolean::New(log_warning(console_msg));
	else if (console_type == "error") return v8::Boolean::New(log_error(console_msg));
	else return v8::Undefined();
}

v8::Handle<v8::Value> setContentTypeNeeded(const v8::Arguments& args) {
	v8::String::Utf8Value flag(args[0]);
	content_type_needed = atoi(ToCString(flag));
	return v8::Undefined();
}

v8::Handle<v8::Value> isContentTypeNeeded(const v8::Arguments& args) {
	return v8::Boolean::New(content_type_needed);
}

// Reads a file into a v8 string.
v8::Handle<v8::Value> ReadFile(const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL) {
		v8::ThrowException(v8::String::New((std::string("Error reading from file: ")+std::string(name)).c_str()));
		return v8::Undefined();
	}
	
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);
	
	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);
	v8::Handle<v8::String> result = v8::String::New(chars, size);
	delete[] chars;
	return result;
}

// Writes string to a file.
void WriteFile(const char* name, const char* content, const int size) {
	FILE* file = fopen(name, "wb");
	if (file == NULL) return;
	
	fwrite(content,1,size,file);
	
	fclose(file);
}

v8::Handle<v8::Value> ReadIO(bool nonblocking) {
	int content_length = 100, current_size = 0, old_flags, retval;
	char cread;
	char* content = NULL;
	v8::Handle<v8::String> result = v8::String::New("",0);
	
	if (nonblocking) { // if non-blocking read requested, peek into stdin to see if we should read or not
		#ifdef _WIN32
		// windows doesn't have fcntl(2), so fake non-blocking stdin peek via a select() with a 20ms timeout
		fd_set rfds;
		struct timeval tv;
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 20*1000;	// 20ms timeout
		retval = select(1, &rfds, NULL, NULL, &tv);
		#else
		// otherwise fnctl(2) should be supported, so use it to set reads to non-blocking, then peek at the stdin stream
		old_flags = fcntl(0, F_GETFL, 0);
		fcntl(0, F_SETFL, old_flags | O_NONBLOCK);		/* set up non-blocking read */
		cread = fgetc(stdin);
		fcntl(0, F_SETFL, old_flags);		/* reset to blocking reads */
		if (cread != EOF) {
			ungetc(cread,stdin);
			retval = 1;
		}
		else retval = 0;
		#endif
	}
	else retval = 1; // force read since not non-blocking
	
	// if stdin has data or we're forcing blocking read, do the read; otherwise, skip reading
	if (retval != 0) {
		do {
			cread = fgetc(stdin);
			if (cread && cread != EOF) {
				if (content==NULL) content = (char*)malloc(content_length);
				else if (current_size == content_length-2) {
					content_length *= 2;
					content = (char*)realloc(content,content_length);
				}
				if (content==NULL) {
					return v8::ThrowException(v8::String::New("Error reading from input"));
				}
				
				content[current_size++] = cread;
			}
		} while (!feof(stdin) && cread && cread != EOF);
		content[current_size] = '\0';
		
		result = v8::String::New(content, current_size);
		free(content);
	}
	
	return result;
}

void WriteIO(const char* content) {
	printf("%s", content);
}

void FlushIO() {
	fflush(stdout);
}

v8::Handle<v8::Value> executeProcess(const v8::Arguments& args) {
	int pid, pipe_to_child[2], pipe_from_child[2], num_args = args.Length(), i, child_stat,
		buffer_size = 1024, buffer_length = 0, read_length = 0
	;
	char tmp_output[1024] = {0};
	bool error = false;
	char cread;
	char* buffer = NULL;
	char** cmd_args;
	v8::Handle<v8::String> result = v8::String::New("",0);
	v8::String::Utf8Value cmd(args[1]);
	
	// take off args[0], since will be sent into stdin of process
	num_args--;
	std::string input_data = ToStdString(args[0]);
	std::string tmp_arg;
	
	// copy the args array (starting at index 1) into cmd_args
	cmd_args = (char**)malloc((num_args+1)*sizeof(char*));
	for (i=0; i<num_args; i++) {
		tmp_arg = ToStdString(args[i+1]);
		cmd_args[i] = (char*)malloc((tmp_arg.length()+1)*sizeof(char));
		strcpy(cmd_args[i],tmp_arg.c_str());
	}
	cmd_args[num_args] = (char *)0;	// mark the end of the args array for execvp()
	
	if (pipe(pipe_to_child) == -1) {
		error = true;
	}
	if (pipe(pipe_from_child) == -1) {
		error = true;
	}
	
	if (!error) {
		pid = fork();
		switch (pid) {
			case -1: // error
				error = true;
			
			case 0: // child process
				dup2(pipe_to_child[0],0);	// redirect pipe read end to be stdin of this child process
				dup2(pipe_from_child[1],1);	// redirect pipe write end to be stdout of this child process
				
				close(pipe_from_child[0]);
				close(pipe_from_child[1]);
				close(pipe_to_child[0]);
				close(pipe_to_child[1]);
				
				execvp(ToCString(cmd),cmd_args);
				exit(1);	// if we get here, the execute didn't work, so bail on child process
			
			default: // parent process
				close(pipe_to_child[0]);
				close(pipe_from_child[1]);
				if (!input_data.empty()) {
					write(pipe_to_child[1],input_data.c_str(),strlen(input_data.c_str()));
				}
				close(pipe_to_child[1]);
				
				waitpid(pid,&child_stat,0);
				if (child_stat != 0) {
					error = true;
				}
				else {
					while ((read_length = read(pipe_from_child[0],&tmp_output,1024)) > 0) {
						if (buffer == NULL) buffer = (char*)malloc(read_length*sizeof(char));
						else if (buffer_length + read_length >= buffer_size-5) {
							buffer_size *= 2;
							buffer = (char*)realloc(buffer,buffer_size);
						}
						if (buffer != NULL) {
							strncpy(&buffer[buffer_length],(const char*)tmp_output,read_length);
							buffer_length += read_length;
						}
						else {
							error = true;
							break;
						}
					}
				}
				close(pipe_from_child[0]);
				if (buffer != NULL) {
					if (!error) {
						result = v8::String::New(buffer, buffer_length);
					}
					free(buffer);
				}
		}
	}
	
	for (i=0; i<num_args; i++) free(cmd_args[i]);
	free(cmd_args);
	
	if (!error) return result;
	return v8::ThrowException(v8::String::New("Error executing process"));
}

void ReportException(v8::TryCatch* try_catch) {
	v8::HandleScope handle_scope;
	std::string exception_string = ToStdString(try_catch->Exception());
	v8::Handle<v8::Message> message = try_catch->Message();
	
	if (content_type_needed) {
		printf("Content-type: text/plain\n\n");
		content_type_needed = 0;
	}
	
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		log_error(exception_string);
	}
	else {
		// Print (filename):(line number):(char position) (message).
		std::string filename_string = ToStdString(message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		int start = message->GetStartColumn();
		std::string sourceline_string = ToStdString(message->GetSourceLine());
		
		log_error(filename_string + ":" + stringify(linenum) + ":" + stringify(start) + " " + exception_string + "\n" + sourceline_string);
	}
}

bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name) {
	v8::HandleScope handle_scope;
	v8::Handle<v8::Value> result = ExecuteStringValue(source, name);
	
	return !result.IsEmpty();
}

v8::Handle<v8::Value> ExecuteStringValue(v8::Handle<v8::String> source, v8::Handle<v8::Value> name) {
	v8::HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()) {
		// Print errors that happened during compilation.
		ReportException(&try_catch);
		return v8::Undefined();
	}
	else {
		return handle_scope.Close(script->Run());
	}
}

std::vector<std::string> split_path(std::string path) {
	std::vector<std::string> ret(2);
	int pos = path.find_last_of("/");
	ret[0] = path.substr(0,pos) + "/";
	ret[1] = path.substr(pos+1);
	return ret;
}

std::string get_canonical_path(std::string cwd, std::string path) {
	// if path is relative, prepend cwd
	if (path[0] != '/') {
		path = cwd + ((cwd[(cwd.length()-1)] != '/') ? "/" : "") + path;
	}
	
	// now, remove any null directories (ie, './') from the path string
	int null_dir_pos; 
	if (path.substr(0,2) == "./") path = path.substr(2);
	while ((null_dir_pos = path.find("/./")) != std::string::npos) {
		path = path.replace(null_dir_pos,3,"/");
	}
	
	return path;
}

v8::Handle<v8::Value> getCfgVal(std::string cfg_name) {
	v8::HandleScope handle_scope;
	v8::Context::Scope scope(backchannel_context);
	
	// prepare getCfg() parameter
	v8::Handle<v8::Value> getcfg_args[1] = {v8::String::New(cfg_name.c_str())};

	// run getCfg()
	v8::TryCatch try_catch;
	v8::Handle<v8::Value> res = getcfg_func->Call(backchannel_context->Global(), 1, getcfg_args);
	if (!res.IsEmpty()) {
		return handle_scope.Close(res);
	}
	else {
		return v8::Undefined();
	}
}

v8::Handle<v8::Object> getCfgObj() {
	return v8::Handle<v8::Object>::Cast(getCfgVal(""));
}

bool setCfgVal(std::string cfg_name, v8::Handle<v8::Value> cfg_val) {
	v8::HandleScope handle_scope;
	v8::Context::Scope scope(backchannel_context);
	
	// prepare setCfg() parameters
	v8::Handle<v8::Value> setcfg_args[2] = {v8::String::New(cfg_name.c_str()), cfg_val};

	// run setCfg()
	v8::TryCatch try_catch;
	v8::Handle<v8::Value> res = setcfg_func->Call(backchannel_context->Global(), 2, setcfg_args);
	if (res.IsEmpty()) {
		ReportException(&try_catch);	// setCfg() failed
		return false;
	}
	return true;
}

std::string formatLogMessageBasic(std::vector< std::pair<std::string,std::string> > msg, std::string format) {
	std::string formatted = "";
	std::string log_format;
	
	if (format == "*") {
		log_format = "common";
	}
	else {
		log_format = format;
	}
		
	for(std::vector< std::pair<std::string,std::string> >::iterator ii=msg.begin(); ii!=msg.end(); ++ii) {
		// default ("common") format: display each value, with space as separator
		formatted += ((!formatted.empty()) ? " " : "") + ii->second;
	}

	return formatted;
}

std::string formatLogMessage(std::vector< std::pair<std::string,std::string> > msg, std::string format) {
	if (!backchannel_ready) {
		return formatLogMessageBasic(msg,format);
	}

	v8::HandleScope handle_scope;
	v8::Context::Scope scope(backchannel_context);

	std::string formatted = "";
	std::string log_format = "";
	v8::Handle<v8::Object> obj_inject;

	if (format == "*") {
 		obj_inject = v8::Object::New();
 		log_format = ToStdString(getCfgVal(std::string("log_format")));
	}
	else {
		log_format = format;
	}
		
	for(std::vector< std::pair<std::string,std::string> >::iterator ii=msg.begin(); ii!=msg.end(); ++ii) {
		if (log_format == "json") {
			obj_inject->Set(v8::String::New(ii->first.c_str()), v8::String::New(ii->second.c_str()));
		}
		else { // default ("common") format: display each value, with space as separator
			formatted += ((!formatted.empty()) ? " " : "") + ii->second;
		}
	}
	
	if (log_format == "json") {
		v8::Handle<v8::Value> obj_args[1] = {obj_inject};
		
		// run JSON.stringify() on object to format it to JSON
		v8::TryCatch try_catch;
		v8::Handle<v8::Value> res = jsonstringify_func->Call(backchannel_context->Global(), 1, obj_args);
		if (res.IsEmpty()) {
			ReportException(&try_catch);	// JSON.stringify() failed to be called
			return "";
		}
		
		formatted = ToStdString(res);
	}

	return formatted;
}

off_t file_size(std::string filename) {
	struct stat fileinfo; 
	int res; 

	res = stat(filename.c_str(),&fileinfo);
	if (res == 0) { 
		return fileinfo.st_size;
	}
	else { 
		return -1;
	} 
}

bool file_exists(std::string filename) {
	return file_size(filename) >= 0;
}

void init_log_file(std::string log_format, std::string filename, FILE* fh) {
	if (file_size(filename) == 0) {
		std::vector< std::pair<std::string,std::string> > msg_vec;
		msg_vec.push_back(std::pair<std::string,std::string>("type","Init"));
		msg_vec.push_back(std::pair<std::string,std::string>("timestamp",current_timestamp()));
		std::string msg;
		if (log_format == "json") msg = "[\n " + formatLogMessage(msg_vec) + "\n]\n";
		else msg = formatLogMessage(msg_vec) + "\n";
		
		fwrite(msg.c_str(),sizeof(char),msg.length(),fh);
		fflush(fh);
	}
}

void write_to_log_file(std::string log_format, std::string str, FILE* fh) {
	if (log_format == "json") {
		fseek(fh, -3, SEEK_END);
	}
	fwrite(str.c_str(),sizeof(char),str.length(),fh);
	fflush(fh);
}

bool logMessage(std::string message, int msg_type) {
// msg_type:
//	0: notice
//	1: console
//	2: warning
//	3: error

	if (message.empty()) {
		return false;
	}
	
	std::string write_msg = message;
	
	if (backchannel_ready) {
		v8::HandleScope handle_scope;
		v8::Context::Scope scope(backchannel_context);
		
		v8::Handle<v8::Object> cfg = getCfgObj();
		if (!cfg.IsEmpty() && !cfg->IsUndefined()) {
	 		std::string log_format = ToStdString(cfg->Get(v8::String::New("log_format")));
			std::string file_log_file = ToStdString(cfg->Get(v8::String::New("file_log_file")));
			std::string file_log_error_file = ToStdString(cfg->Get(v8::String::New("file_log_error_file")));
			int log_level = cfg->Get(v8::String::New("log_level"))->IntegerValue();
			int error_log_level = cfg->Get(v8::String::New("error_log_level"))->IntegerValue();
			
			if (log_format == "json") write_msg = ",\n " + write_msg + "\n]\n";
			else write_msg += "\n";
			
			if (cfg->Get(v8::String::New("logging"))->BooleanValue()) {
				if (cfg->Get(v8::String::New("file_logging"))->BooleanValue()) {
					if ((msg_type == 0 || msg_type == 1) && (log_level <= msg_type)) {
						init_log_file(log_format,file_log_file,file_log_file_handle);
						write_to_log_file(log_format,write_msg,file_log_file_handle);
					}
					else if (msg_type >= 2 && (error_log_level+2) <= msg_type) {
						init_log_file(log_format,file_log_error_file,file_log_error_file_handle);
						write_to_log_file(log_format,write_msg,file_log_error_file_handle);
					}
				}
				if (cfg->Get(v8::String::New("system_event_logging"))->BooleanValue()) {
				
				}
			}
			if (msg_type >= 3 && (!cfg->Get(v8::String::New("logging"))->BooleanValue() || cfg->Get(v8::String::New("display_errors"))->BooleanValue())) {
				if (content_type_needed) {
					printf("Content-type: text/plain\n\n");
					content_type_needed = 0;
				}
				if (cfg->Get(v8::String::New("display_errors"))->BooleanValue()) { // forced display of error
					printf("%s\n",message.c_str());
				}
				else {	// error was not logged or displayed to stdout, so at least send to stderr
					fprintf(stderr, "%s\n",message.c_str());
				}
			}
			return true;
		}
		return false;
	}
	else { // logging before backchannel is ready
		// so, only log errors (and above), to stderr
		if (msg_type >= 3) {
			if (content_type_needed) {
				printf("Content-type: text/plain\n\n");
				content_type_needed = 0;
			}
			fprintf(stderr, "%s\n",message.c_str());
		}
		return true;
	}
}

std::string stringify(int num) {
	std::ostringstream o;
	o << num;
	return o.str();
}

std::string left_pad_str(std::string str, int toLength, char pad) {
	if (toLength < 1 || str.length() >= toLength) return str;
	return std::string(toLength-str.length(), pad) + str;
}

std::string current_timestamp() {
	std::string dow[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	std::string mon[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	std::string ts;
	time_t rawtime;
	struct tm* timeinfo;
	std::string tz;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	std::string tzoffset = stringify(0-(timezone/36));
	if (tzoffset[0] != '-') tzoffset = "+" + tzoffset;
	tzoffset = "GMT" + tzoffset.substr(0,1) + left_pad_str(tzoffset.substr(1),4,'0');
	
	ts = dow[timeinfo->tm_wday] + " " + mon[timeinfo->tm_mon] + " " + stringify(timeinfo->tm_mday) + " " + stringify(1900+timeinfo->tm_year);
	ts += " " + left_pad_str(stringify(timeinfo->tm_hour),2,'0') + ":" + left_pad_str(stringify(timeinfo->tm_min),2,'0') + ":" + left_pad_str(stringify(timeinfo->tm_sec),2,'0');
	ts += " " + tzoffset + " (" + std::string(*tzname) + ")";

	return ts;
}

bool log(std::string msg, int msg_type=0, int errorNumber) {
// msg_type:
//	0: notice
//	1: console
//	2: warning
//	3: error

	std::string types[4] = {"Notice", "Console", "Warning", "Error"};

	if (!msg.empty()) {
		std::vector< std::pair<std::string,std::string> > msg_vec;

		msg_vec.push_back(std::pair<std::string,std::string>("type",types[msg_type]));
		msg_vec.push_back(std::pair<std::string,std::string>("timestamp",current_timestamp()));
		if (msg_type >= 3 && errorNumber > 0) {
			msg_vec.push_back(std::pair<std::string,std::string>("code",stringify(errorNumber)));
		}
		msg_vec.push_back(std::pair<std::string,std::string>("msg",msg));
		
		return logMessage(formatLogMessage(msg_vec),msg_type);
	}
	else {
		return false;
	}
}

bool log_notice(std::string msg) { return log(msg,0); }
bool log_console(std::string msg) { return log(msg,1); }
bool log_warning(std::string msg) { return log(msg,2); }
bool log_error(std::string msg, int error_number) { return log(msg,3,error_number); }
bool log_fatal(std::string msg, int error_number) { return log(msg,3,error_number); }


int RunMain(int argc, char* argv[]) {

	// initialize the timezone info
	tzset();

// ***** SETUP *****
	// get the current working directory and full executable path
	std::string path_cwd(gnu_getcwd());
	std::string path_exec(argv[0]);
	
	// parse path parts
	std::string path_parts_str = get_canonical_path(path_cwd,path_exec);
	std::vector<std::string> path_parts = split_path(path_parts_str);
	std::string exec_path_directory = path_parts[0];
	std::string exec_path_name = path_parts[1];
	
	bikechain_root_path_directory = exec_path_directory.substr(0,exec_path_directory.find_last_of("/",exec_path_directory.length()-2)+1);
	
	log_notice("Starting BikechainJS in " + bikechain_root_path_directory);
	
	// start the V8 environment
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	v8::HandleScope handle_scope;
	
	v8::Handle<v8::String> bikechain_root_path = v8::String::New(bikechain_root_path_directory.c_str());

// ***** BACKCHANNEL *****
	// set up the backchannel controller (for processing config file and formatting log messages)
	v8::Handle<v8::ObjectTemplate> backchannel_global = v8::ObjectTemplate::New();
	backchannel_context = v8::Context::New(NULL, backchannel_global);
	{
		v8::Context::Scope scope(backchannel_context);
		
		// prepare JSON serializer
		jsonstringify_func = v8::Handle<v8::Function>::Cast(ExecuteStringValue(v8::String::New("(function(j){return JSON.stringify(j);})"),v8::String::New("inline::code")));
		if (jsonstringify_func.IsEmpty() || jsonstringify_func->IsUndefined()) {
			log_fatal("Failed preparing engine backchannel JSON utility");
			return 1;
		}
		
		// load JSON.minify() utility
		tmpval = ReadFile((bikechain_root_path_directory+"misc/minify.json.js").c_str());
		if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
			log_fatal("Failed loading engine backchannel: `minify.json.js`");
			return 1;
		}
		v8::Handle<v8::String> json_minify_source = v8::Handle<v8::String>::Cast(tmpval);
		if (!ExecuteString(json_minify_source, v8::String::New("minify.json.js"))) {
			log_fatal("Failed parsing engine backchannel: `minify.json.js`");
			return 1;
		}
	
		// load String.trim() utility
		tmpval = ReadFile((bikechain_root_path_directory+"misc/string.trim.js").c_str());
		if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
			log_fatal("Failed loading engine backchannel; `string.trim.js`");
			return 1;
		}
		v8::Handle<v8::String> string_trim_source = v8::Handle<v8::String>::Cast(tmpval);
		if (!ExecuteString(string_trim_source, v8::String::New("string.trim.js"))) {
			log_fatal("Failed parsing engine backchannel: `string.trim.js`");
			return 1;
		}

		// load/prepare backchannel controller
		tmpval = ReadFile((exec_path_directory+"backchannel.js").c_str());
		if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
			log_fatal("Failed loading engine backchannel: `backchannel.js`");
			return 1;
		}
		v8::Handle<v8::Function> backchannel_func = v8::Handle<v8::Function>::Cast(ExecuteStringValue(v8::Handle<v8::String>::Cast(tmpval),v8::String::New("backchannel.js")));
		if (backchannel_func.IsEmpty() || backchannel_func->IsUndefined()) {
			log_fatal("Failed parsing engine backchannel: `backchannel.js`");
			return 1;
		}
			
		// read/parse config file
		tmpval = ReadFile((exec_path_directory+exec_path_name+".json").c_str());
		if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
			log_fatal("Failed loading engine configuration: `" + exec_path_name + ".json`");
			return 1;
		}

		// prepare backchannel init parameters
		v8::Handle<v8::Object> backchannel_inject = v8::Object::New();
		backchannel_inject->Set(v8::String::New("cfg"), v8::Handle<v8::String>::Cast(tmpval));
		v8::Handle<v8::Value> backchannel_args[1] = {backchannel_inject};

		// run backchannel init
		v8::TryCatch try_catch;
		v8::Handle<v8::Value> res = backchannel_func->Call(backchannel_context->Global(), 1, backchannel_args);
		if (res.IsEmpty()) {
			ReportException(&try_catch);
			log_fatal("Failed initializing backchannel");
			return 1;
		}

		// store references to backchannel configuration get/set JS functions
		getcfg_func = v8::Handle<v8::Function>::Cast(ExecuteStringValue(v8::String::New("getCfg"),v8::String::New("inline::code")));
		if (getcfg_func.IsEmpty() || getcfg_func->IsUndefined()) {
			log_fatal("Failed preparing engine configuration management");
			return 1;
		}
		v8::TryCatch try_catch2;
		setcfg_func = v8::Handle<v8::Function>::Cast(ExecuteStringValue(v8::String::New("setCfg"),v8::String::New("inline::code")));
		if (setcfg_func.IsEmpty() || setcfg_func->IsUndefined()) {
			ReportException(&try_catch2);
			log_fatal("Failed preparing engine configuration management");
			return 1;
		}

		// manage engine configuration
		v8::Handle<v8::Object> tmp_cfg = getCfgObj();
		if (!tmp_cfg.IsEmpty() && !tmp_cfg->IsUndefined()) {
			// canonicalize the `file_log_*` paths from the engine configuration
			std::string file_log_file = get_canonical_path(bikechain_root_path_directory,ToStdString(tmp_cfg->Get(v8::String::New("file_log_file"))));
			std::string file_log_error_file = get_canonical_path(bikechain_root_path_directory,ToStdString(tmp_cfg->Get(v8::String::New("file_log_error_file"))));
			
			// push the updated path values back into the configuration cache	
			if (!setCfgVal("file_log_file", v8::String::New(file_log_file.c_str()))) {
				log_fatal("Failed normalizing engine configuration");
				return 1;
			}
			if (!setCfgVal("file_log_error_file", v8::String::New(file_log_error_file.c_str()))) {
				log_fatal("Failed normalizing engine configuration");
				return 1;
			}
			
			// make sure the log files exist and are properly initialized
			if (!file_exists(file_log_file)) file_log_file_handle = fopen(file_log_file.c_str(),"w+t");
			else file_log_file_handle = fopen(file_log_file.c_str(),"r+t");
			if (file_log_file_handle == NULL) {
				log_fatal("Failed opening log file: `" + file_log_file + "`");
				return 1;
			}
			chmod(file_log_file.c_str(),0664);
			
			if (!file_exists(file_log_error_file)) file_log_error_file_handle = fopen(file_log_error_file.c_str(),"w+t");
			else file_log_error_file_handle = fopen(file_log_error_file.c_str(),"r+t");
			if (file_log_error_file_handle == NULL) {
				log_fatal("Failed opening error log file: `" + file_log_error_file + "`");
				return 1;
			}
			chmod(file_log_error_file.c_str(),0664);
		}
		else {
			log_fatal("Failed executing engine configuration management");
			return 1;
		}
		
		backchannel_ready = true;
	}
		
	
// ***** ENGINE *****
	v8::Handle<v8::ObjectTemplate> engine_global = v8::ObjectTemplate::New();
	engine_context = v8::Context::New(NULL, engine_global);
	{
		v8::Context::Scope scope(engine_context);
	
		// load engine source
		tmpval = ReadFile((exec_path_directory+exec_path_name+".js").c_str());
		if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
			log_fatal("Failed loading engine: `" + exec_path_name + ".js`");
			return 1;
		}
		
		v8::Handle<v8::Function> engine_func = v8::Handle<v8::Function>::Cast(ExecuteStringValue(v8::Handle<v8::String>::Cast(tmpval),v8::String::New(std::string(exec_path_name+".js").c_str())));
		if (engine_func.IsEmpty() || engine_func->IsUndefined()) {
			log_fatal("Failed parsing engine: `" + exec_path_name + ".js`");
			return 1;
		}
		
		// prepare engine init parameters
		v8::Handle<v8::Object> engine_inject = v8::Object::New();
		engine_inject->Set(v8::String::New("__IORead__"), v8::FunctionTemplate::New(ioRead)->GetFunction());
		engine_inject->Set(v8::String::New("__IOWrite__"), v8::FunctionTemplate::New(ioWrite)->GetFunction());
		engine_inject->Set(v8::String::New("__IOFlush__"), v8::FunctionTemplate::New(ioFlush)->GetFunction());
		engine_inject->Set(v8::String::New("__FSRead__"), v8::FunctionTemplate::New(fsRead)->GetFunction());
		engine_inject->Set(v8::String::New("__FSWrite__"), v8::FunctionTemplate::New(fsWrite)->GetFunction());
		engine_inject->Set(v8::String::New("__SysExec__"), v8::FunctionTemplate::New(sysExec)->GetFunction());
		engine_inject->Set(v8::String::New("__SysExit__"), v8::FunctionTemplate::New(sysExit)->GetFunction());
		engine_inject->Set(v8::String::New("__Console__"), v8::FunctionTemplate::New(sysConsole)->GetFunction());
		engine_inject->Set(v8::String::New("__SetContentTypeNeeded__"), v8::FunctionTemplate::New(setContentTypeNeeded)->GetFunction());
		engine_inject->Set(v8::String::New("__IsContentTypeNeeded__"), v8::FunctionTemplate::New(isContentTypeNeeded)->GetFunction());
		v8::Handle<v8::Value> engine_args[2] = {bikechain_root_path,engine_inject};
		
		// run engine
		v8::TryCatch try_catch;
		v8::Handle<v8::Value> res = engine_func->Call(engine_context->Global(), 2, engine_args);
		if (res.IsEmpty()) {
			ReportException(&try_catch);	// engine failed to init
			log_fatal("Failed initializing engine");
			return 1;
		}
		
		log_notice("Engine and backchannel ready");

	
// ***** COMMAND-LINE *****
		
		std::string filename_str;

		// run parameter .js files
		for (int i = 1; i < argc; i++) {
			v8::HandleScope handle_scope;
			const char* str = argv[i];
			
			// Use all arguments as names of files to load and run.
			filename_str = get_canonical_path(path_cwd,std::string(str));
			v8::Handle<v8::String> file_name = v8::String::New(filename_str.c_str());
			
			log_notice("Loading `" + filename_str + "`");

			// load and run file specified			
			tmpval = ReadFile(filename_str.c_str());
			if (tmpval.IsEmpty() || tmpval->IsUndefined()) {
				log_fatal("Failed loading file: `" + filename_str + "`");
				return 1;
			}
			if (!ExecuteString(v8::Handle<v8::String>::Cast(tmpval),file_name)) {
				log_fatal("Failed executing file: `" + filename_str + "`");
				return 1;
			}
		}
	}
	
	return 0;
}

int main(int argc, char* argv[]) {
	int result = RunMain(argc, argv);
	v8::V8::Dispose();
	fclose(file_log_file_handle);
	fclose(file_log_error_file_handle);
	return result;
}

