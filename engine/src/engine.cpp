/*  BikechainJS (engine)
v0.0.1.4b (c) Kyle Simpson
MIT License
*/

#include <v8.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef _WIN32
#include <sys/time.h>
#endif

#include <engine.h>

int content_type_needed = 0;

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
	exit(status);
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
	if (file == NULL) return v8::ThrowException(v8::String::New((std::string("Error reading from file: ")+std::string(name)).c_str()));	
	
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
	v8::String::Utf8Value v8_input_data(args[0]);
	std::string input_data(ToCString(v8_input_data));
	
	// copy the args array (starting at index 1) into cmd_args
	cmd_args = (char**)malloc((num_args+1)*sizeof(char*));
	for (i=0; i<num_args; i++) {
		v8::String::Utf8Value tmp(args[i+1]);
		cmd_args[i] = (char*)malloc(strlen(ToCString(tmp))*sizeof(char));
		strcpy(cmd_args[i],ToCString(tmp));
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
				if (input_data != "") {
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
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Handle<v8::Message> message = try_catch->Message();
	
	if (content_type_needed) {
		printf("Content-type: text/plain\n\n");
	}
	
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		printf("%s\n", exception_string);
	}
	else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		printf("%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		printf("%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			printf(" ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			printf("^");
		}
		printf("\n");
	}
}

bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name) {
	v8::HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()) {
		// Print errors that happened during compilation.
		ReportException(&try_catch);
		return false;
	}
	else {
		v8::Handle<v8::Value> result = script->Run();
		if (result.IsEmpty()) {
			// Print errors that happened during execution.
			ReportException(&try_catch);
			return false;
		}
		return true;
	}
}

int RunMain(int argc, char* argv[]) {
	// get the current working directory and full executable path
	char* cwd_str = gnu_getcwd();
	std::string path_cwd(cwd_str);
	free(cwd_str);
	std::string path_exec(argv[0]);
	std::string full_path = path_cwd + "/" + path_exec;
	
	// start the V8 environment
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	v8::HandleScope handle_scope;
	
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	
	v8::Handle<v8::Context> context = v8::Context::New(NULL, global);
	v8::Context::Scope context_scope(context);
	
	v8::Handle<v8::String> source = v8::Handle<v8::String>::Cast(ReadFile((full_path+".js").c_str()));
	v8::Handle<v8::Script> compiled_source = v8::Script::Compile(source);
	if (compiled_source.IsEmpty()) {
		printf("Error reading engine.js\n");
		return 1;
	}
	v8::Handle<v8::Value> enginejs = compiled_source->Run();
	v8::Handle<v8::Function> enginejs_func = v8::Handle<v8::Function>::Cast(enginejs);
	
	v8::Handle<v8::Object> inject = v8::Object::New();
	inject->Set(v8::String::New("__IORead__"), v8::FunctionTemplate::New(ioRead)->GetFunction());
	inject->Set(v8::String::New("__IOWrite__"), v8::FunctionTemplate::New(ioWrite)->GetFunction());
	inject->Set(v8::String::New("__IOFlush__"), v8::FunctionTemplate::New(ioFlush)->GetFunction());
	inject->Set(v8::String::New("__FSRead__"), v8::FunctionTemplate::New(fsRead)->GetFunction());
	inject->Set(v8::String::New("__FSWrite__"), v8::FunctionTemplate::New(fsWrite)->GetFunction());
	inject->Set(v8::String::New("__SysExec__"), v8::FunctionTemplate::New(sysExec)->GetFunction());
	inject->Set(v8::String::New("__SysExit__"), v8::FunctionTemplate::New(sysExit)->GetFunction());
	inject->Set(v8::String::New("__SetContentTypeNeeded__"), v8::FunctionTemplate::New(setContentTypeNeeded)->GetFunction());
	inject->Set(v8::String::New("__IsContentTypeNeeded__"), v8::FunctionTemplate::New(isContentTypeNeeded)->GetFunction());
	v8::Handle<v8::Value> args[1] = {inject};
	
	enginejs_func->Call(context->Global(), 1, args);
	
	for (int i = 1; i < argc; i++) {
		const char* str = argv[i];
		// Use all arguments as names of files to load and run.
		v8::HandleScope handle_scope;
		v8::Handle<v8::String> file_name = v8::String::New(str);
		v8::Handle<v8::String> source = v8::Handle<v8::String>::Cast(ReadFile((path_cwd + "/" + std::string(str)).c_str()));
		if (source.IsEmpty()) {
			printf("Error reading '%s'\n", str);
			return 1;
		}
		if (!ExecuteString(source, file_name))
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	int result = RunMain(argc, argv);
	v8::V8::Dispose();
	return result;
}

