/*  BikechainJS (engine)
	v0.0.2 (c) Kyle Simpson
	MIT License
*/

const char* v8ToCString(const v8::String::Utf8Value& value);
std::string v8ToStdString(v8::Handle<v8::Value> value);

v8::Handle<v8::Value> ioRead(const v8::Arguments& args);
v8::Handle<v8::Value> ioWrite(const v8::Arguments& args);
v8::Handle<v8::Value> ioErrWrite(const v8::Arguments& args);
v8::Handle<v8::Value> fsRead(const v8::Arguments& args);
v8::Handle<v8::Value> fsWrite(const v8::Arguments& args);
v8::Handle<v8::Value> sysExec(const v8::Arguments& args);
v8::Handle<v8::Value> sysExit(const v8::Arguments& args);
v8::Handle<v8::Value> sysConsole(const v8::Arguments& args);
v8::Handle<v8::Value> setContentTypeNeeded(const v8::Arguments& args);
v8::Handle<v8::Value> isContentTypeNeeded(const v8::Arguments& args);
v8::Handle<v8::Value> executeCode(const v8::Arguments& args);
v8::Handle<v8::Value> executeModule(const v8::Arguments& args);

v8::Handle<v8::Value> ReadFile(const char* name);
void WriteFile(const char* name, const char* content, const int size);
v8::Handle<v8::Value> ReadIO(bool nonblocking);
void WriteIO(const char* content);
void WriteIOErr(const char* content);
void FlushIO();
v8::Handle<v8::Value> executeProcess(const v8::Arguments& args);

void ReportException(v8::TryCatch* handler);
bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::String> name);
v8::Handle<v8::Value> ExecuteStringValue(v8::Handle<v8::String> source, v8::Handle<v8::String> name);

std::vector<std::string> split_path(std::string path);
std::string escape_whitespace(std::string str);
std::string get_canonical_path(std::string cwd, std::string path);

v8::Handle<v8::Value> getCfgVal(std::string cfg_name);
v8::Handle<v8::Object> getCfgObj(void);
bool setCfgVal(std::string cfg_name, v8::Handle<v8::Value> cfg_val);
std::string formatLogMessageBasic(std::vector< std::pair<std::string,std::string> > msg, std::string format="*");
std::string formatLogMessage(std::vector< std::pair<std::string,std::string> > msg, std::string format="*");
bool logMessage(std::string message, int msg_type=0);

off_t file_size(std::string filename);
bool file_exists(std::string filename);
void init_log_file(std::string log_format, std::string filename, FILE* fh);
void write_to_log_file(std::string log_format, std::string str, FILE* fh);

std::string stringify(int num);
std::string left_pad_str(std::string str, int to_length, char pad=' ');
std::string current_timestamp();

bool log(std::string msg, int msg_type=0, int error_number=0);
bool log_notice(std::string msg);
bool log_console(std::string msg);
bool log_warning(std::string msg);
bool log_error(std::string msg, int error_number=0);
bool log_fatal(std::string msg, int error_number=0);

void cleanup();
