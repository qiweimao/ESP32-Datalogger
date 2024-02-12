#include "utils.h"
#include "api_interface.h"
#include "Secrets.h"

void startFileServer();
void Dir(AsyncWebServerRequest * request);
void Directory();
void UploadFileSelect();
void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void File_Stream();
void File_Delete();
void Handle_File_Delete(String filename);
void File_Rename();
void Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
String processor(const String& var);
void notFound(AsyncWebServerRequest *request);
void Handle_File_Download();
String getContentType(String filenametype);
void Select_File_For_Function(String title, String function);
void SelectInput(String Heading, String Command, String Arg_name);
int GetFileSize(String filename);
void Home();
void LogOut();
void Display_New_Page();
void Page_Not_Found();
void Display_System_Info();
String ConvBinUnits(int bytes, int resolution);
String EncryptionType(wifi_auth_mode_t encryptionType);

String HTML_Header();
String HTML_Footer();