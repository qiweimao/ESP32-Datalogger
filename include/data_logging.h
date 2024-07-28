#ifndef DATALOGGING_H
#define DATALOGGING_H

enum LogErrorCode {
  LOG_SUCCESS,
  FILE_OPEN_ERROR,
  FILE_WRITE_ERROR,
  PAUSED
};  // Add more error codes as needed

void log_data_init();

#endif