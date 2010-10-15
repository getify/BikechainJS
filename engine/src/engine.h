/*  BikechainJS (engine)
	v0.0.1.3 (c) Kyle Simpson
	MIT License
*/

const char* ToCString(const v8::String::Utf8Value& value);

v8::Handle<v8::Value> ioRead();
v8::Handle<v8::Value> ioWrite(const v8::Arguments& args);
v8::Handle<v8::Value> fsRead(const v8::Arguments& args);
v8::Handle<v8::Value> fsWrite(const v8::Arguments& args);
v8::Handle<v8::Value> sysExec(const v8::Arguments& args);
v8::Handle<v8::Value> sysExit(const v8::Arguments& args);
v8::Handle<v8::Value> contentTypeNeeded(const v8::Arguments& args);
v8::Handle<v8::Value> isContentTypeNeeded(const v8::Arguments& args);

v8::Handle<v8::Value> ReadFile(const char* name);
void WriteFile(const char* name, const char* content, const int size);
v8::Handle<v8::Value> ReadIO(bool nonblocking);
void WriteIO(const char* content);
void FlushIO();
v8::Handle<v8::Value> executeProcess(const v8::Arguments& args);

void ReportException(v8::TryCatch* handler);
bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name, bool print_result, bool report_exceptions);
