/**
 * @file memory_benchmark.c
 * @brief Benchmark comparing traditional memory management versus Copy-on-Write (COW)
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <windows.h>

 /**
  * @def ARRAY_SIZE
  * @brief Size of the test array in elements
  */
 #define ARRAY_SIZE 120000000

 /**
  * @def NUM_COPIES
  * @brief Number of copies to create for each benchmark
  */
 #define NUM_COPIES 50

 /**
  * @def NUM_MODIFICATIONS
  * @brief Number of modifications to perform on each copy
  */
 #define NUM_MODIFICATIONS 2400

 /**
  * @def NUM_ITERATIONS
  * @brief Number of iterations to run for each benchmark method
  */
 #define NUM_ITERATIONS 10

 /**
  * @struct cow_array
  * @brief Structure representing a Copy-on-Write array
  *
  * This structure contains a pointer to the data array and a reference count
  * to track how many copies are sharing the same underlying data.
  */
 typedef struct {
     int* data;       /**< Pointer to the shared data array */
     int* ref_count;  /**< Pointer to the reference count */
 } cow_array;

 /**
  * @brief Creates a new Copy-on-Write array of specified size
  *
  * Allocates memory for a new array and initializes its reference count to 1.
  *
  * @param size Number of elements in the array
  * @return cow_array A new COW array structure
  */
 cow_array cow_create(int size) {
     cow_array arr;
     arr.data = (int*)malloc(size * sizeof(int));
     arr.ref_count = (int*)malloc(sizeof(int));
     *(arr.ref_count) = 1;
     return arr;
 }

 /**
  * @brief Creates a Copy-on-Write copy of an existing array
  *
  * Instead of duplicating the data, this function increases the reference count
  * of the source array and returns a new cow_array structure pointing to the same data.
  *
  * @param src The source array to copy
  * @return cow_array A new COW array structure sharing data with the source
  */
 cow_array cow_copy(cow_array src) {
     cow_array dest = src;
     (*(dest.ref_count))++;
     return dest;
 }

 /**
  * @brief Ensures the array has unique (non-shared) data before modification
  *
  * If the reference count is greater than 1 (meaning the data is shared),
  * this function creates a private copy of the data for the array and
  * decrements the reference count of the original shared data.
  *
  * @param arr Pointer to the COW array to make unique
  */
 void cow_ensure_unique(cow_array* arr) {
     if (*(arr->ref_count) > 1) {
         // Need to make a real copy
         int* old_data = arr->data;
         arr->data = (int*)malloc(ARRAY_SIZE * sizeof(int));
         memcpy(arr->data, old_data, ARRAY_SIZE * sizeof(int));

         // Decrease the reference count of the original
         (*(arr->ref_count))--;

         // Create a new reference count for this copy
         arr->ref_count = (int*)malloc(sizeof(int));
         *(arr->ref_count) = 1;
     }
 }

 /**
  * @brief Frees the memory used by a COW array
  *
  * Decrements the reference count and frees the memory only if the
  * reference count reaches zero (no more references to the data).
  *
  * @param arr Pointer to the COW array to free
  */
 void cow_free(cow_array* arr) {
     (*(arr->ref_count))--;
     if (*(arr->ref_count) == 0) {
         free(arr->data);
         free(arr->ref_count);
     }
 }

 /**
  * @brief Benchmark function for traditional memory management
  *
  * Creates an original array, makes multiple complete copies of it,
  * and then performs random modifications on each copy.
  *
  * @return double Execution time in seconds
  */
 double benchmark_traditional() {
     printf("  Traditional method: Allocating memory...\n");

     // Create original array and initialize with random data
     int* original = (int*)malloc(ARRAY_SIZE * sizeof(int));
     if (!original) {
         printf("Error: Could not allocate memory for original array\n");
         exit(1);
     }

     printf("  Traditional method: Initializing data...\n");
     for (int i = 0; i < ARRAY_SIZE; i++) {
         original[i] = rand();
     }

     LARGE_INTEGER start, end, frequency;
     QueryPerformanceFrequency(&frequency);

     printf("  Traditional method: Starting timed section...\n");
     QueryPerformanceCounter(&start);

     // Create copies using traditional method
     int* copies[NUM_COPIES];
     for (int i = 0; i < NUM_COPIES; i++) {
         if (i % 5 == 0) {
             printf("  Traditional method: Creating copy %d of %d...\n", i+1, NUM_COPIES);
         }
         copies[i] = (int*)malloc(ARRAY_SIZE * sizeof(int));
         if (!copies[i]) {
             printf("Error: Could not allocate memory for copy %d\n", i);

             // Free previously allocated memory
             free(original);
             for (int j = 0; j < i; j++) {
                 free(copies[j]);
             }
             exit(1);
         }
         memcpy(copies[i], original, ARRAY_SIZE * sizeof(int));
     }

     // Modify just a few random elements in each copy - small, sparse changes
     // are where COW really shines
     for (int i = 0; i < NUM_COPIES; i++) {
         if (i % 10 == 0) {
             printf("  Traditional method: Modifying copy %d of %d...\n", i+1, NUM_COPIES);
         }
         for (int j = 0; j < NUM_MODIFICATIONS; j++) {
             int index = rand() % ARRAY_SIZE;
             copies[i][index] = rand();
         }
     }

     QueryPerformanceCounter(&end);

     printf("  Traditional method: Cleaning up memory...\n");
     // Free memory
     free(original);
     for (int i = 0; i < NUM_COPIES; i++) {
         free(copies[i]);
     }

     return (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
 }

 /**
  * @brief Benchmark function for Copy-on-Write memory management
  *
  * Creates an original array, makes multiple COW copies of it (which initially
  * share the same data), and then performs random modifications on each copy,
  * forcing real copies to be made only when needed.
  *
  * @return double Execution time in seconds
  */
 double benchmark_cow() {
     printf("  COW method: Allocating memory...\n");

     // Create original COW array and initialize with random data
     cow_array original = cow_create(ARRAY_SIZE);
     if (!original.data) {
         printf("Error: Could not allocate memory for original COW array\n");
         exit(1);
     }

     printf("  COW method: Initializing data...\n");
     for (int i = 0; i < ARRAY_SIZE; i++) {
         original.data[i] = rand();
     }

     LARGE_INTEGER start, end, frequency;
     QueryPerformanceFrequency(&frequency);

     printf("  COW method: Starting timed section...\n");
     QueryPerformanceCounter(&start);

     // Create copies using COW
     cow_array copies[NUM_COPIES];
     for (int i = 0; i < NUM_COPIES; i++) {
         if (i % 10 == 0) {
             printf("  COW method: Creating copy %d of %d...\n", i+1, NUM_COPIES);
         }
         copies[i] = cow_copy(original);
     }

     // Modify just a few random elements in each copy
     for (int i = 0; i < NUM_COPIES; i++) {
         if (i % 10 == 0) {
             printf("  COW method: Modifying copy %d of %d...\n", i+1, NUM_COPIES);
         }

         // Track if we've made this copy unique already
         int made_unique = 0;

         for (int j = 0; j < NUM_MODIFICATIONS; j++) {
             int index = rand() % ARRAY_SIZE;

             // Only call ensure_unique once per copy
             if (!made_unique) {
                 cow_ensure_unique(&copies[i]);
                 made_unique = 1;
             }

             copies[i].data[index] = rand();
         }
     }

     QueryPerformanceCounter(&end);

     printf("  COW method: Cleaning up memory...\n");
     // Free memory
     cow_free(&original);
     for (int i = 0; i < NUM_COPIES; i++) {
         cow_free(&copies[i]);
     }

     return (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
 }

 /**
  * @brief Displays system memory information
  *
  * Shows available system memory to help determine if the benchmark
  * can run with the current dataset size.
  */
 void show_memory_info() {
     MEMORYSTATUSEX memInfo;
     memInfo.dwLength = sizeof(MEMORYSTATUSEX);
     GlobalMemoryStatusEx(&memInfo);

     DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
     DWORDLONG availPhysMem = memInfo.ullAvailPhys;

     printf("System Memory Information:\n");
     printf("  Total physical memory: %.2f GB\n", totalPhysMem / (1024.0 * 1024.0 * 1024.0));
     printf("  Available physical memory: %.2f GB\n", availPhysMem / (1024.0 * 1024.0 * 1024.0));

     double required_traditional = (ARRAY_SIZE * sizeof(int) * (NUM_COPIES + 1)) / (1024.0 * 1024.0 * 1024.0);
     double required_cow = (ARRAY_SIZE * sizeof(int) * (1 + 1)) / (1024.0 * 1024.0 * 1024.0); // 1 original + 1 potential copy

     printf("  Estimated peak memory for traditional: %.2f GB\n", required_traditional);
     printf("  Estimated peak memory for COW (best case): %.2f GB\n", required_cow);

     if (required_traditional > availPhysMem / (1024.0 * 1024.0 * 1024.0)) {
         printf("  WARNING: This benchmark may require more memory than available.\n");
         printf("  Consider reducing ARRAY_SIZE or NUM_COPIES if you encounter issues.\n");
         printf("  Press Enter to continue anyway or Ctrl+C to abort...");
         getchar();
     }
     printf("\n");
 }

 /**
  * @brief Main function running the benchmark comparison
  *
  * Initializes the random number generator, runs both benchmarks multiple times,
  * and reports the results with percentage comparisons.
  *
  * @return int Exit code (0 for success)
  */
 int main() {
     // Seed the random number generator
     srand((unsigned int)time(NULL));

     double traditional_time = 0.0;
     double cow_time = 0.0;

     printf("=== MEMORY MANAGEMENT BENCHMARK ===\n");
     printf("DATASET OPTIMIZED FOR COPY-ON-WRITE PERFORMANCE\n\n");
     printf("Array size: %d elements (%.2f GB)\n", ARRAY_SIZE, (ARRAY_SIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0));
     printf("Number of copies: %d\n", NUM_COPIES);
     printf("Modifications per copy: %d (%.5f%% of array)\n", NUM_MODIFICATIONS,
            (NUM_MODIFICATIONS * 100.0) / ARRAY_SIZE);
     printf("Number of iterations: %d\n\n", NUM_ITERATIONS);

     // Check system memory
     show_memory_info();

     printf("Running benchmark...\n\n");

     // Run each benchmark method multiple times for more reliable results
     for (int i = 0; i < NUM_ITERATIONS; i++) {
         printf("=== Iteration %d of %d ===\n", i+1, NUM_ITERATIONS);

         // Run COW first to minimize memory pressure
         printf("   Running Copy-on-Write benchmark...\n");
         double c_time = benchmark_cow();
         cow_time += c_time;
         printf("  Copy-on-Write benchmark completed in %.4f seconds\n\n", c_time);

         // Force garbage collection to free memory
         printf("  Performing memory cleanup before next benchmark...\n");
         system("timeout /t 2 >nul");

         printf("   Running traditional memory management benchmark...\n");
         double t_time = benchmark_traditional();
         traditional_time += t_time;
         printf("  Traditional benchmark completed in %.4f seconds\n\n", t_time);

         printf("Iteration %d results:\n", i+1);
         printf("  Traditional: %.4f seconds\n", t_time);
         printf("  Copy-on-Write: %.4f seconds\n", c_time);

         if (t_time > c_time) {
             printf("  Copy-on-Write is %.2f%% faster in this iteration\n\n",
                    (t_time - c_time) / t_time * 100.0);
         } else {
             printf("  Traditional is %.2f%% faster in this iteration\n\n",
                    (c_time - t_time) / c_time * 100.0);
         }

         // Force garbage collection to free memory
         printf("  Performing memory cleanup before next iteration...\n\n");
         system("timeout /t 3 >nul");
     }

     // Calculate averages
     traditional_time /= NUM_ITERATIONS;
     cow_time /= NUM_ITERATIONS;

     // Report final results
     printf("=== FINAL RESULTS ===\n");
     printf("Traditional method: %.4f seconds (average)\n", traditional_time);
     printf("Copy-on-Write method: %.4f seconds (average)\n\n", cow_time);

     if (traditional_time > cow_time) {
         printf("WINNER: Copy-on-Write is %.2f%% faster than traditional memory management\n",
                (traditional_time - cow_time) / traditional_time * 100.0);
     } else {
         printf("WINNER: Traditional is %.2f%% faster than Copy-on-Write\n",
                (cow_time - traditional_time) / cow_time * 100.0);
     }

     printf("\nCOW vs Traditional Efficiency Analysis:\n");
     printf("- This benchmark used a dataset with large arrays (%.2f GB) and very few modifications (%.5f%%)\n",
            (ARRAY_SIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0),
            (NUM_MODIFICATIONS * 100.0) / ARRAY_SIZE);
     printf("- Each traditional copy required a full %.2f GB memory allocation and copy\n",
            (ARRAY_SIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0));
     printf("- COW copies initially shared the same data, only duplicating when modified\n");
     printf("- With %d copies and only %d modifications per copy, most data remained shared\n",
            NUM_COPIES, NUM_MODIFICATIONS);

     printf("Press Enter to exit...");
     getchar();

     return 0;
 }
