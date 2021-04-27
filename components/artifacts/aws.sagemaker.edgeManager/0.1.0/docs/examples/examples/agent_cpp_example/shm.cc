#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <functional>
#include <numeric>
#include <stdio.h>
#include <string.h>

#include "shm.hh"


#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

const int FAILURE = -1;

void* INVALID_SEGMENT_ADDRESS = (void*)-1;

std::string generate_key(size_t length) {
    srand((unsigned int)time(NULL));
    auto randchar = []() -> char {
        const char charset[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

long createSHM(const unsigned long segment_size) {
    long segment_id = shmget(ftok(generate_key(10).c_str(), 'N'), segment_size, IPC_CREAT | 0666);
    if (segment_id == FAILURE) {
        std::cout << "Error: failed to create shm segment with size . " << segment_size
                  << " errno - " << errno << " , error - " << strerror(errno) << std::endl;
    }
    return segment_id;
}

void* attachSHMWrite(unsigned long segment_id) {
#ifdef __linux__
    auto result = shmat(segment_id, NULL, 0);
    if (result == INVALID_SEGMENT_ADDRESS) {
        std::cout << "Error: failed to attach shared memory segment. id - " << segment_id
                  << " errno - " << errno << " , error - " << strerror(errno) << std::endl;
    }
    return result;
#endif
}

bool makeSHMReadOnly(const unsigned long segment_id) {
    // change permission to read-only
    struct shmid_ds segment_region;
    if (shmctl(segment_id, IPC_STAT, &segment_region) == FAILURE) {
        std::cout << "Error: failed to get stats for shm segment. id - " << segment_id
                  << " errno - " << errno << " , error - " << strerror(errno) << std::endl;
        return false;
    }
    segment_region.shm_perm.mode = S_IREAD;
    if (shmctl(segment_id, IPC_SET, &segment_region) == FAILURE) {
        std::cout << "Error: failed to set shm segment id - " << segment_id << " to be read-only"
                  << std::endl;
        return false;
    }
    return true;
}