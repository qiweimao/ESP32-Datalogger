void mqtt_initialize();
void mqtt_reconnect();
void mqtt_process_file();
void mqtt_process_file_in_batches();
bool mqtt_process_file(const char* filename);
bool mqtt_process_folder(String folderPath, String extension);