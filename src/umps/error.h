/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef UMPS_ERROR_H
#define UMPS_ERROR_H

#include <stdexcept>
#include <string>

struct RuntimeError : public std::runtime_error {
    RuntimeError(const std::string& what)
        : std::runtime_error(what) {}
};

struct FileError : public std::runtime_error {
    FileError(const std::string& fileName)
        : std::runtime_error("Error accessing `" + fileName + "'") {}
};

// Error hooks

// FIXME: These were the old error handling/signalling functions.  The
// problem is their use mostly doesn't even fit the new architecture;
// the goal is to gradually replace all of the calls to smtg
// appropriate.

void Panic(const char* message);

void ShowAlert(const char* s1, const char* s2, const char* s3);

void ShowAlertQuit(const char* s1, const char* s2, const char* s3);

#endif // UMPS_ERROR_H
