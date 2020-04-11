#ifndef WEB_SERVER_HIDDEN_H
#define WEB_SERVER_HIDDEN_H

void handleListdir();
void handleCredentials();
void handleScanNetworks();
void handleNetwork();
void handleDebugInfo();
void handleLogger();
void handleReset();
void handleUploadComplete();
void handleUpload();
void handleSendFile();
void handleCpu();
void handleReadDeviceId();
void handleReadMemory();
void handleBackupCerts();
void handleRestoreCerts();
void handleDeviceInfo();
void handleDeviceState();
void handleSchedule();
void handleNotFound();

#endif