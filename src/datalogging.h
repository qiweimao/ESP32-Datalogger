#ifndef DATALOGGING_H
#define DATALOGGING_H

extern const char *filename;
extern const int LOG_INTERVAL;

enum LogErrorCode {
  LOG_SUCCESS,
  FILE_OPEN_ERROR,
  FILE_WRITE_ERROR
  // Add more error codes as needed
};

LogErrorCode logData();

#endif