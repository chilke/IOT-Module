#ifndef WEB_SERVER_HIDDEN_H
#define WEB_SERVER_HIDDEN_H

void sendNotAllowed();
void handleListdir();
void handleCredentials();
void handleScanNetworks();
void handleNetwork();
void handleDebugInfo();
void handleLogger();
void handleReset();
void handleUpload();
void handleNotFound();

#endif