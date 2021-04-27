#pragma once

#include <iostream>
#include <vector>

extern void* INVALID_SEGMENT_ADDRESS;

std::string generate_key(size_t length);

long createSHM(const unsigned long segment_size);

void* attachSHMWrite(const unsigned long segment_id);

bool makeSHMReadOnly(const unsigned long segment_id);