/**
 * @file disk_monitor.c
 * @brief Windows C drive read/write ratio monitoring utility
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <windows.h>
 #include <time.h>

 /**
  * @brief Duration in seconds to monitor disk activity
  */
 #define MONITORING_DURATION_SEC 600

 /**
  * @brief Structure to hold disk I/O statistics
  */
 typedef struct {
     ULONGLONG readOperations;  /**< Number of read operations */
     ULONGLONG writeOperations; /**< Number of write operations */
     ULONGLONG readBytes;       /**< Number of bytes read */
     ULONGLONG writeBytes;      /**< Number of bytes written */
 } IOStats;

 /**
  * @brief Retrieves current disk performance statistics for C drive
  *
  * Uses the Windows DeviceIoControl API with IOCTL_DISK_PERFORMANCE to
  * collect actual disk I/O statistics for the C drive.
  *
  * @param[out] stats Pointer to IOStats structure to store the results
  * @return TRUE if statistics were retrieved successfully, FALSE otherwise
  */
 BOOL GetDriveStats(IOStats* stats) {
     // Open handle to C drive
     HANDLE hDevice = CreateFileA(
         "\\\\.\\C:",
         GENERIC_READ,
         FILE_SHARE_READ | FILE_SHARE_WRITE,
         NULL,
         OPEN_EXISTING,
         0,
         NULL
     );

     if (hDevice == INVALID_HANDLE_VALUE) {
         printf("Error opening C drive: (error %d)\n", GetLastError());
         return FALSE;
     }

     DISK_PERFORMANCE diskPerformance;
     DWORD bytesReturned;

     BOOL success = DeviceIoControl(
         hDevice,
         IOCTL_DISK_PERFORMANCE,
         NULL,
         0,
         &diskPerformance,
         sizeof(diskPerformance),
         &bytesReturned,
         NULL
     );

     if (!success) {
         printf("Error getting disk performance info for C drive: (error %d)\n", GetLastError());
         CloseHandle(hDevice);
         return FALSE;
     }

     stats->readOperations = diskPerformance.ReadCount;
     stats->writeOperations = diskPerformance.WriteCount;
     stats->readBytes = diskPerformance.BytesRead.QuadPart;
     stats->writeBytes = diskPerformance.BytesWritten.QuadPart;

     CloseHandle(hDevice);
     return TRUE;
 }

 /**
  * @brief Main program entry point
  *
  * Monitors C drive activity for a specified duration, then calculates
  * and displays read/write operation ratios and byte transfer ratios.
  *
  * @return 0 on success, 1 on failure
  */
 int main() {
     printf("C Drive Read/Write Ratio Monitor for Windows\n");
     printf("-------------------------------------------\n");
     printf("This program will monitor C drive activity for %d seconds\n", MONITORING_DURATION_SEC);
     printf("and report the ratio of read operations to write operations.\n\n");

     // Get initial stats
     IOStats initialStats, finalStats;
     if (!GetDriveStats(&initialStats)) {
         printf("Failed to get initial drive statistics.\n");
         printf("Note: This program requires administrator privileges.\n");
         printf("Please run as administrator and try again.\n");
         return 1;
     }

     // Wait for the monitoring period
     printf("Monitoring disk activity for %d seconds...\n", MONITORING_DURATION_SEC);
     time_t startTime = time(NULL);
     time_t endTime = startTime + MONITORING_DURATION_SEC;

     // Show a progress indicator
     int progress = 0;
     printf("[");
     for (int i = 0; i < 50; i++) printf(" ");
     printf("]\r[");

     while (time(NULL) < endTime) {
         int newProgress = (int)(50.0 * (time(NULL) - startTime) / MONITORING_DURATION_SEC);
         while (progress < newProgress && progress < 50) {
             printf("#");
             progress++;
         }
         fflush(stdout);
         Sleep(100);
     }
     printf("]\n\n");

     // Get final stats
     if (!GetDriveStats(&finalStats)) {
         printf("Failed to get final drive statistics.\n");
         return 1;
     }

     // Calculate differences
     ULONGLONG readOps = finalStats.readOperations - initialStats.readOperations;
     ULONGLONG writeOps = finalStats.writeOperations - initialStats.writeOperations;
     ULONGLONG readBytes = finalStats.readBytes - initialStats.readBytes;
     ULONGLONG writeBytes = finalStats.writeBytes - initialStats.writeBytes;

     // Calculate ratios
     double opRatio = (writeOps > 0) ? ((double)readOps / writeOps) : 0;
     double bytesRatio = (writeBytes > 0) ? ((double)readBytes / writeBytes) : 0;

     // Print results
     printf("Results:\n");
     printf("--------\n");
     printf("Read operations:  %llu\n", readOps);
     printf("Write operations: %llu\n", writeOps);
     printf("Read/Write ratio: %.2f:1\n\n", opRatio);

     printf("Bytes read:       %llu bytes (%.2f MB)\n", readBytes, readBytes / (1024.0 * 1024.0));
     printf("Bytes written:    %llu bytes (%.2f MB)\n", writeBytes, writeBytes / (1024.0 * 1024.0));
     printf("Bytes ratio:      %.2f:1\n\n", bytesRatio);

     if (readOps > 0 && writeOps > 0) {
         printf("For every 10,000 read operations, there are approximately %.0f write operations\n",
                10000.0 * writeOps / readOps);
         printf("For every 10,000 reads, there are approximately %.0f writes\n",
                10000.0 / opRatio);
     }

     printf("Press Enter to exit...");
     getchar();

     return 0;
 }
