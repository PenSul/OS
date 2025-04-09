#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Layer structure representing a container image layer
 */
typedef struct {
    char id[64];            /**< Layer identifier */
    size_t size_mb;         /**< Layer size in MB */
    char content_hash[64];  /**< Content hash for layer identification */
} Layer;

/**
 * @brief Container structure representing a running container
 */
typedef struct {
    char id[64];     /**< Container identifier */
    Layer** layers;  /**< Array of layers */
    int layer_count; /**< Number of layers */
} Container;

/**
 * @brief Creates a base Ubuntu layer
 * @return Pointer to the created layer
 */
Layer* create_ubuntu_layer() {
    Layer* layer = (Layer*)malloc(sizeof(Layer));
    strcpy(layer->id, "ubuntu:latest");
    layer->size_mb = 120;
    strcpy(layer->content_hash, "sha256:ubuntu-base-layer");
    return layer;
}

/**
 * @brief Creates a container-specific layer
 * @param container_id Container identifier
 * @return Pointer to the created layer
 */
Layer* create_container_layer(int container_id) {
    Layer* layer = (Layer*)malloc(sizeof(Layer));
    sprintf(layer->id, "container-%d-layer", container_id);
    layer->size_mb = 3;
    sprintf(layer->content_hash, "sha256:container-%d-unique", container_id);
    return layer;
}

/**
 * @brief Creates containers without Copy-on-Write
 * @param containers Array to store container pointers
 * @param count Number of containers to create
 */
void create_containers_no_cow(Container** containers, int count) {
    for (int i = 0; i < count; i++) {
        containers[i] = (Container*)malloc(sizeof(Container));
        sprintf(containers[i]->id, "container-%d", i+1);

        containers[i]->layer_count = 1;
        containers[i]->layers = (Layer**)malloc(sizeof(Layer*) * containers[i]->layer_count);
        containers[i]->layers[0] = create_ubuntu_layer();
    }
}

/**
 * @brief Creates containers with Copy-on-Write
 * @param containers Array to store container pointers
 * @param count Number of containers to create
 * @param shared_ubuntu Shared Ubuntu base layer
 */
void create_containers_cow(Container** containers, int count, Layer* shared_ubuntu) {
    for (int i = 0; i < count; i++) {
        containers[i] = (Container*)malloc(sizeof(Container));
        sprintf(containers[i]->id, "container-%d", i+1);

        containers[i]->layer_count = 2;
        containers[i]->layers = (Layer**)malloc(sizeof(Layer*) * containers[i]->layer_count);

        containers[i]->layers[0] = shared_ubuntu;
        containers[i]->layers[1] = create_container_layer(i+1);
    }
}

/**
 * @brief Calculates total storage without Copy-on-Write
 * @param containers Array of container pointers
 * @param count Number of containers
 * @return Total storage size in MB
 */
size_t calculate_storage_no_cow(Container** containers, int count) {
    size_t total = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < containers[i]->layer_count; j++) {
            total += containers[i]->layers[j]->size_mb;
        }
    }
    return total;
}

/**
 * @brief Calculates total storage with Copy-on-Write
 * @param containers Array of container pointers
 * @param count Number of containers
 * @param shared_ubuntu Shared Ubuntu base layer
 * @return Total storage size in MB
 */
size_t calculate_storage_cow(Container** containers, int count, Layer* shared_ubuntu) {
    size_t total = shared_ubuntu->size_mb;

    for (int i = 0; i < count; i++) {
        for (int j = 1; j < containers[i]->layer_count; j++) {
            total += containers[i]->layers[j]->size_mb;
        }
    }

    return total;
}

/**
 * @brief Cleans up containers without Copy-on-Write
 * @param containers Array of container pointers
 * @param count Number of containers
 */
void cleanup_containers_no_cow(Container** containers, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < containers[i]->layer_count; j++) {
            free(containers[i]->layers[j]);
        }
        free(containers[i]->layers);
        free(containers[i]);
    }
}

/**
 * @brief Cleans up containers with Copy-on-Write
 * @param containers Array of container pointers
 * @param count Number of containers
 */
void cleanup_containers_cow(Container** containers, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = 1; j < containers[i]->layer_count; j++) {
            free(containers[i]->layers[j]);
        }
        free(containers[i]->layers);
        free(containers[i]);
    }
}

/**
 * @brief Main function demonstrating storage comparison
 * @return Exit code
 */
int main() {
    const int CONTAINER_COUNT = 10;

    printf("Docker Container Storage Layer Comparison\n");
    printf("=========================================\n\n");

    printf("Scenario: 10 containers based on a 120MB Ubuntu image\n");
    printf("Each container has 3MB of unique data\n\n");

    Container** containers_no_cow = (Container**)malloc(sizeof(Container*) * CONTAINER_COUNT);
    create_containers_no_cow(containers_no_cow, CONTAINER_COUNT);

    size_t storage_no_cow = calculate_storage_no_cow(containers_no_cow, CONTAINER_COUNT);

    Layer* shared_ubuntu = create_ubuntu_layer();

    Container** containers_cow = (Container**)malloc(sizeof(Container*) * CONTAINER_COUNT);
    create_containers_cow(containers_cow, CONTAINER_COUNT, shared_ubuntu);

    size_t storage_cow = calculate_storage_cow(containers_cow, CONTAINER_COUNT, shared_ubuntu);

    printf("WITHOUT Copy-on-Write:\n");
    printf("----------------------\n");
    printf("Each container has its own full copy of the Ubuntu image\n");
    printf("Total storage: %zu MB\n", storage_no_cow);
    printf("Storage per container: %zu MB\n\n", storage_no_cow / CONTAINER_COUNT);

    printf("WITH Copy-on-Write (Docker's approach):\n");
    printf("--------------------------------------\n");
    printf("All containers share the same Ubuntu base image\n");
    printf("Each container only stores its unique data (3MB)\n");
    printf("Total storage: %zu MB\n", storage_cow);
    printf("Effective storage per container: %.1f MB\n\n", (float)storage_cow / CONTAINER_COUNT);

    printf("STORAGE EFFICIENCY:\n");
    printf("-----------------\n");
    printf("Storage saved with CoW: %zu MB\n", storage_no_cow - storage_cow);
    printf("Storage reduction: %.1f%%\n",
           100.0 * (1.0 - ((double)storage_cow / (double)storage_no_cow)));

    cleanup_containers_no_cow(containers_no_cow, CONTAINER_COUNT);
    cleanup_containers_cow(containers_cow, CONTAINER_COUNT);
    free(shared_ubuntu);
    free(containers_no_cow);
    free(containers_cow);

    printf("Press Enter to exit...");
    getchar();

    return 0;
}
