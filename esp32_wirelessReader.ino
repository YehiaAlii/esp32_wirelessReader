#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <pgmspace.h>  // Include for PROGMEM to store static data in flash memory

#define SD_CS_PIN 5 // SD Card Configuration
#define SWITCH_PIN 4 // Use GPIO 4 for the switch

WebServer server(80); // WiFi Server Object

// Flag to track the state of the Access Point (AP)
bool isApActive = false;
bool sdInitialized = false;

// Minified and stored in PROGMEM to save RAM
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>ESP32 File Navigator</title><style>body{margin:0;font-family:Arial,sans-serif;background-color:#f4f4f9;color:#333;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh}main{text-align:center;padding:2rem;box-shadow:0 4px 15px rgba(0,0,0,0.1);border-radius:12px;background-color:#fff;max-width:400px;width:90%;margin:20px auto}main h2{font-size:2rem;color:#2699D3;margin-bottom:1rem}main p{font-size:1.2rem;color:#666;margin-bottom:2rem}a{display:inline-block;text-decoration:none;color:#fff;background:linear-gradient(90deg,#2699D3,#1C6BA0);padding:12px 24px;border-radius:8px;font-size:1rem;font-weight:bold;box-shadow:0 3px 8px rgba(0,0,0,0.2);transition:background 0.3s ease-in-out,transform 0.2s ease-in-out}a:hover{background:linear-gradient(90deg,#1C6BA0,#154B75);transform:scale(1.05)}footer{margin-top:1.5rem;font-size:0.9rem;color:#999;text-align:center}footer p{margin:0}</style></head><body><main><h2>Welcome!</h2><p>Manage your files with ease.</p><a href="/listWeb">Browse Files</a></main><footer><p>&copy; 2024 ESP32 File Navigator by Yehia Ali</p></footer></body></html>
)rawliteral";

// Interrupt Service Routine (ISR) to handle switch state changes
void IRAM_ATTR handleSwitchChange() {
  if (digitalRead(SWITCH_PIN) == LOW) { // Check if the switch is pressed
    if (!isApActive) {
      startSoftAP(); // Start the Access Point if it's not active
    }
  } else {
    if (isApActive) {
      stopSoftAP(); // Stop the Access Point if it's active
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 initialized. Enter '1' to start AP, '0' to stop AP.");

  // Configure the switch pin with internal pull-up
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // Attach interrupt to the switch pin to detect changes
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), handleSwitchChange, CHANGE);  // Attach interrupt

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Failed to initialize SD card.");
  } else {
    sdInitialized = true;
    Serial.println("SD card initialized successfully.");
  }
}

// Function to start the Access Point
void startSoftAP() {
  if (!isApActive) {
    WiFi.softAP("ESP32-AP", "12345678"); // Set SSID and password
    Serial.println("Soft AP started");
    isApActive = true;

    // Define server routes and handlers
    server.on("/", []() {
      server.send_P(200, "text/html", index_html);
    });
    server.on("/listWeb", handleListWeb);
    server.on("/createFolder", HTTP_POST, handleCreateFolder);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/delete", HTTP_GET, handleDelete);

    server.begin(); // Start the web server
    Serial.println("HTTP server started");
  }
}

// Function to stop the Access Point
void stopSoftAP() {
  if (isApActive) {
    WiFi.softAPdisconnect(true); // Disconnect the AP
    Serial.println("Soft AP stopped");
    isApActive = false;
    server.stop(); // Stop the web server
  }
}

// Function to determine the MIME type based on file extension
String getMimeType(const String& filepath) {
  if (filepath.endsWith(".pdf")) return "application/pdf";
  if (filepath.endsWith(".jpg") || filepath.endsWith(".jpeg")) return "image/jpeg";
  if (filepath.endsWith(".png")) return "image/png";
  if (filepath.endsWith(".txt")) return "text/plain";
  if (filepath.endsWith(".mp4")) return "video/mp4";
  return "application/octet-stream"; // Default MIME type
}

// Handler for listing files and directories on the SD card
void handleListWeb() {
  if (!sdInitialized) {
    server.send(500, "text/plain", "SD card not initialized.");
    return;
  }

  String path = "/";
  if (server.hasArg("path")) {
    path = server.arg("path");
    if (!path.startsWith("/")) { 
      path = "/" + path;
    }
  }

  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    server.send(500, "text/plain", "Invalid directory.");
    return;
  }

  String response = "<!DOCTYPE html><html><head><title>File Browser</title><style>body{font-family:Arial,sans-serif;background-color:#f4f4f9;color:#333}ul{list-style-type:none;padding:0;max-width:600px;margin:20px auto;background:#fff;border-radius:8px;box-shadow:0 4px 15px rgba(0,0,0,0.1)}li{padding:10px;display:flex;align-items:center;justify-content:space-between;font-size:1.1rem}li:not(.no-border){border-bottom:1px solid #e0e0e0}a{text-decoration:none;color:#333;flex-grow:1;display:block;padding:5px 0}a:hover{color:#2699D3}.button-container{display:flex}.button-container a{flex:1;text-align:center;margin:0;padding:10px;border-radius:5px}.go-back{font-weight:bold;color:#2699D3;background-color:#e0f7fa;border-right:4px solid #ccc}.create-folder{font-weight:bold;color:#2699D3;background-color:#e0f7fa}.go-back:hover,.create-folder:hover{background-color:#b2ebf2;color:#007c91}.action-buttons{display:flex;gap:10px}.button{padding:5px 10px;text-align:center;text-decoration:none;border-radius:5px;font-size:0.9rem}.download{background-color:#2699D3;color:white}.download:hover{background-color:#1C6BA0}.delete{background-color:#e57373;color:white}.delete:hover{background-color:#d32f2f}</style></head><body><ul>";

  // Add "Go Back" and "Create Folder" buttons in the same line
  response += "<li class=\"no-border button-container\">";
  if (path != "/") {
    String parentPath = path.substring(0, path.lastIndexOf('/'));
    if (parentPath.isEmpty()) parentPath = "/";
    response += "<a class=\"go-back\" href=\"/listWeb?path=" + parentPath + "\">Go Back</a>";
  }
  response += "<a class=\"create-folder\" onclick=\"createFolder()\">Create Folder</a>";
  response += "</li>";

  // List files and directories
  File file = dir.openNextFile();
  while (file) {
    String name = String(file.name());
    if (file.isDirectory()) {
      // Folder entry
      response += "<li><a href=\"/listWeb?path=" + path + "/" + name + "\">" + name + "</a><div class=\"action-buttons\"><a class=\"button delete\" href=\"/delete?name=" + path + "/" + name + "\">Delete</a></div></li>";
    } else {
      // File entry
      response += "<li><a href=\"/download?name=" + path + "/" + name + "\">" + name + "</a><div class=\"action-buttons\"><a class=\"button download\" href=\"/download?name=" + path + "/" + name + "&download=1\">Download</a><a class=\"button delete\" href=\"/delete?name=" + path + "/" + name + "\">Delete</a></div></li>";
    }
    file = dir.openNextFile();
  }
  // JavaScript for creating a folder
  response += "</ul><script>function createFolder(){var folderName=prompt('Enter the name of the new folder:');if(folderName){var form=document.createElement('form');form.method='POST';form.action='/createFolder';var input=document.createElement('input');input.type='hidden';input.name='folderName';input.value=folderName;form.appendChild(input);var pathInput=document.createElement('input');pathInput.type='hidden';pathInput.name='path';pathInput.value='" + path + "';form.appendChild(pathInput);document.body.appendChild(form);form.submit();}}</script></body></html>";
  server.send(200, "text/html", response);
}

// Handler for creating a new folder on the SD card
void handleCreateFolder() {
  if (!sdInitialized) {
    server.send(500, "text/plain", "SD card not initialized.");
    return;
  }

  if (!server.hasArg("folderName") || !server.hasArg("path")) {
    server.send(400, "text/plain", "Folder name or path missing.");
    return;
  }

  String folderName = server.arg("folderName");
  String path = server.arg("path");
  // Construct the full path for the new folder
  String fullPath = path + "/" + folderName;

  if (SD.mkdir(fullPath.c_str())) {
    server.send(200, "text/html", "<script>alert('Folder created successfully.'); window.location = document.referrer;</script>");
  } else {
    server.send(500, "text/html", "<script>alert('Failed to create folder.'); window.history.back();</script>");
  }
}

// Handler for downloading a file from the SD card
void handleDownload() {
  if (!sdInitialized) {
    server.send(500, "text/plain", "SD card not initialized.");
    return;
  }

  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "File name missing.");
    return;
  }

  String filepath = server.arg("name");
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    server.send(404, "text/plain", "File not found.");
    return;
  }
  // get the Mime Type of the file
  String mimeType = getMimeType(filepath);
  // Check if the 'download' parameter is set to force a download
  bool forceDownload = server.hasArg("download") && server.arg("download") == "1";
  String contentDisposition = forceDownload ? "attachment" : "inline";
  // Set headers
  server.setContentLength(file.size());
  server.sendHeader("Content-Type", mimeType);
  server.sendHeader("Content-Disposition", contentDisposition + "; filename=\"" + filepath.substring(filepath.lastIndexOf('/') + 1) + "\"");
  server.send(200, mimeType, ""); // Initiate transfer
  // Stream the file
  uint8_t buffer[1024];
  size_t bytesRead;
  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    server.client().write(buffer, bytesRead);
  }
  file.close();
}

// Handler for deleting a file or an empty folder from the SD card
void handleDelete() {
  if (!sdInitialized) {
      server.send(500, "text/plain", "SD card not initialized.");
      return;
  }
    if (!server.hasArg("name")) {
        server.send(400, "text/plain", "File or folder name missing.");
        return;
    }

    String filepath = server.arg("name");
    // Check if the file or folder exists
    if (!SD.exists(filepath)) {
    server.send(404, "text/html", "<script>alert('File or folder not found.'); window.history.back();</script>");
        return;
    }

    File target = SD.open(filepath);
    if (target.isDirectory()) {
        // Check if the directory is empty
        File check = target.openNextFile();
        if (check) {
            // Directory is not empty
            server.send(400, "text/html", "<script>alert('Cannot delete a non-empty directory.'); window.history.back();</script>");
            target.close();
            return;
        }
        target.close();
        // Try removing the empty directory
        if (SD.rmdir(filepath.c_str())) {
        server.send(200, "text/html", "<script>alert('Empty folder deleted successfully.'); window.location = document.referrer;</script>");
        } else {
        server.send(500, "text/html", "<script>alert('Failed to delete the empty folder.'); window.history.back();</script>");
        }
    } else {
        target.close();
        // Try removing a file
        if (SD.remove(filepath)) {
      server.send(200, "text/html", "<script>alert('File deleted successfully.'); window.location = document.referrer;</script>");
        } else {
      server.send(500, "text/html", "<script>alert('Failed to delete the file.'); window.history.back();</script>");
        }
    }
}

void loop() {
  // Handle HTTP requests only if AP is active
  if (isApActive) {
    server.handleClient();
  }
}