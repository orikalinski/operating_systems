#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <zconf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>

const char VALID_CHARS[4] = {'G', 'M', 'K', 'B'};


bool isValidDataAmount(char *str) {
    int i = 0;
    for (; str[i + 1] != '\0'; i++)
        if (!isdigit(str[i])) return false;
    for (int j = 0; j < 4; j++)
        if (str[i] == VALID_CHARS[j]) return true;
    return false;
}

long long parseDataAmountIntoInteger(char *dataAmount){
    char *dataType;
    long long bytesNumber = strtol(dataAmount, &dataType, 10);
    int dataTypeSize = 0;
    if (dataType[0] == 'G')
        dataTypeSize = 1073741824;
    if (dataType[0] == 'M')
        dataTypeSize = 1048576;
    if (dataType[0] == 'K')
        dataTypeSize = 1024;
    if (dataType[0] == 'B')
        dataTypeSize = 1;
    return dataTypeSize * bytesNumber;
}

int filterBuffer(char *buffer, char* filteredBuffer, int bufferSize, int lastPosition){
    size_t filteredSize = strlen(filteredBuffer);
    int j = (int)filteredSize;
    int i = lastPosition;
    for (; buffer[i] != '\0'; i++){
        if (j >= bufferSize) break;
        if ((int)buffer[i] >= 32 && (int)buffer[i] <= 126) {
            filteredBuffer[j] = buffer[i];
            j++;
        }
    }
    filteredBuffer[j] = '\0';
    return i;
}

bool filterFile(long long bytesNumber, char *inputFileName, char *outputFileName){
    int inputFile;
    if ((inputFile = open(inputFileName, O_RDONLY)) < 0) {
        printf("Couldn't open file: %s\n", inputFileName);
        return false;
    }
    int outputFile;
    if ((outputFile = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC)) < 0){
        close(inputFile);
        printf("Couldn't open file: %s\n", outputFileName);
        return false;
    }

    size_t bufferSize;
    size_t readBufferSize;
    if (bytesNumber >= 1048576)
        bufferSize = 4096;
    else
        bufferSize = 512;
    long long fileSize = lseek(inputFile, 0, SEEK_END);
    lseek(inputFile, 0, SEEK_SET);
    readBufferSize = fileSize < bufferSize ? (size_t)fileSize : bufferSize;
    readBufferSize = bytesNumber < readBufferSize ? (size_t)bytesNumber : readBufferSize;
    bufferSize = bytesNumber < bufferSize ? (size_t)bytesNumber : bufferSize;
    char *buffer = (char *)malloc(readBufferSize + 1);
    char *filteredBuffer = (char *)malloc(bufferSize + 1);
    filteredBuffer[0] = '\0';  // in order to avoid valgrind crap warning of conditional jump issues
    long long currentBytesSize = 0;
    long long numOfBytesWritten = 0;
    size_t filteredBufferSize = 0;
    int lastPosition = 0;
    ssize_t readSize;

    while (currentBytesSize < bytesNumber){
        readBufferSize = (bytesNumber - currentBytesSize) < readBufferSize ? (size_t)(bytesNumber - currentBytesSize)
                                                                           : readBufferSize;
        readSize = read(inputFile, buffer, readBufferSize);
        buffer[readSize] = '\0';
        if (readSize == 0) {
            lseek(inputFile, 0, SEEK_SET);
            continue;
        }
        lastPosition = filterBuffer(buffer, filteredBuffer, (int)bufferSize, lastPosition);
        filteredBufferSize = strlen(filteredBuffer);
        if (filteredBufferSize >= bufferSize) {
            write(outputFile, filteredBuffer, filteredBufferSize);
            numOfBytesWritten += (int)filteredBufferSize;
            filteredBuffer[0] = '\0';
        }
        if (lastPosition < readBufferSize)
            filterBuffer(buffer, filteredBuffer, (int)bufferSize, lastPosition);
        lastPosition = 0;
        currentBytesSize += readSize;
    }
    filteredBufferSize = strlen(filteredBuffer);
    if (filteredBufferSize > 0) {
        write(outputFile, filteredBuffer, filteredBufferSize);
        numOfBytesWritten += filteredBufferSize;
    }
    free(buffer);
    free(filteredBuffer);
    printf("%lld characters requested, %lld characters read, %lld are printable\n", bytesNumber,
           currentBytesSize, numOfBytesWritten);
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Not enough arguments, number of arguments: %d\n", argc);
        return 1;
    }
    if (!isValidDataAmount(argv[1])){
        printf("Invalid data amount argument: %s\n", argv[1]);
        return 1;
    }
    char *dataAmount = argv[1];
    char *inputFileName = argv[2];
    char *outputFileName = argv[3];
    if (filterFile(parseDataAmountIntoInteger(dataAmount), inputFileName, outputFileName))
        return 0;
    return 1;
}