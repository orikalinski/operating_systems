#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <zconf.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <libgen.h>
#include <sys/time.h>

#define _FILE_OFFSET_BITS 64
#define LARGE_BUFFER_SIZE 4096
#define SMALL_BUFFER_SIZE 512
#define FILENAME_SIZE 256
#define NUM_OF_FILES 100
#define TIME_SIZE 30
#define NUM_OF_FRAGMENTS 3
#define GIGABYTE 1024 * MEGABYTE
#define MEGABYTE 1024 * KILOBYTE
#define KILOBYTE 1024 * BYTE
#define BYTE 1
#define VAULT_FILE_PERMISSIONS 0777
#define PREFIX_DELIMITER "<<<<<<<<"
#define SUFFIX_DELIMITER ">>>>>>>>"
#define DELETION_MARKER "00000000"
#define PREFIX_LENGTH 8
#define SUFFIX_LENGTH 8
#define PREFIX_AND_SUFFIX_LENGTH (PREFIX_LENGTH + SUFFIX_LENGTH)
#define OPEN(fd, filePath, oFlags, permissions) \
    if ((fd = open(filePath, oFlags, permissions)) == -1){ \
        printf("Something went wrong with open()! %s\n", strerror(errno)); \
        return errno; \
    }
#define WRITE(fd, buffer, size) \
    if ((write(fd, buffer, size)) == -1){ \
        printf("Something went wrong with write()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define READ(fd, buffer, size) \
    size_t readSize; \
    if ((readSize = read(fd, buffer, size)) == -1){ \
        printf("Something went wrong with read()! %s\n", strerror(errno)); \
        close(fd); \
        return errno; \
    }
#define READ_CATALOG(fd, vaultFilePath) \
    Catalog catalog1; \
    READ(fd, &catalog1, sizeof(Catalog))
#define WRITE_CATALOG(fd) \
    lseek(fd, 0, SEEK_SET); \
    WRITE(fd, &catalog1, sizeof(Catalog))
#define VALIDATE_NAME(catalog1, fileName, compareValue) \
    if ((fileNameExists(catalog1, fileName) >= 0)==compareValue) { \
        close(vfd); \
        printf("File name %sexists: %s\n", compareValue ? "" : "does not ", fileName); \
        return 1; \
    }

typedef struct block {
    off_t offset;
    ssize_t size;
} Block;

typedef struct repo_metadata {
    ssize_t fileSize;
    time_t creationTimestamp;
    time_t lastModified;
    short numberOfFiles;
    short numberOfGaps;
} RepoMetadata;

typedef struct file_allocation_table {
    char fileName[FILENAME_SIZE];
    ssize_t fileSize;
    mode_t fileProtection;
    time_t insertionDate;
    Block fragments[NUM_OF_FRAGMENTS];
    short numberOfFragments;
} FAT;

typedef struct catalog {
    RepoMetadata repoMD;
    FAT FATArray[NUM_OF_FILES];
    Block gapArray[NUM_OF_FRAGMENTS * NUM_OF_FILES + 1];
} Catalog;


int getCurrentTime(time_t *time1) {
    if (time(time1) < 0){
        printf("Something went wrong with time()! %s\n", strerror(errno)); \
        return errno;
    }
    return 0;
}

char *convertToPrintableDate(time_t time1) {
    char *timeStr;
    if ((timeStr = (char *) malloc(TIME_SIZE)) == NULL) {
        printf("Couldn't allocate memory for the timeStr");
        return NULL;
    }
    struct tm *timeInfo;
    timeInfo = localtime(&time1);
    strftime(timeStr, TIME_SIZE, "%c", timeInfo);
    return timeStr;
}

ssize_t parseDataAmountIntoInteger(char *dataAmount) {
    char *dataType;
    long long bytesNumber = strtol(dataAmount, &dataType, 10);
    int dataTypeSize = 0;
    if (toupper(dataType[0]) == 'G')
        dataTypeSize = GIGABYTE;
    if (toupper(dataType[0]) == 'M')
        dataTypeSize = MEGABYTE;
    if (toupper(dataType[0]) == 'K')
        dataTypeSize = KILOBYTE;
    if (toupper(dataType[0]) == 'B')
        dataTypeSize = BYTE;
    return dataTypeSize * bytesNumber;
}

ssize_t getFreeSpace(Catalog catalog1) {
    ssize_t freeSpace = catalog1.repoMD.fileSize - sizeof(Catalog);
    int i = 0, j;
    for (; i < catalog1.repoMD.numberOfFiles; i++)
        for (j = 0; j < catalog1.FATArray[i].numberOfFragments; j++)
            freeSpace -= catalog1.FATArray[i].fragments[j].size;
    return freeSpace;
}

int getStats(char *filePath, struct stat *stat1) {
    if (stat(filePath, stat1) == 0)
        return 0;
    return 1;
}

int init(char *vaultFilePath, ssize_t fileSize) {
    if (sizeof(Catalog) > fileSize) {
        printf("The file isn't big enough\n");
        return 1;
    }
    int vfd;
    OPEN(vfd, vaultFilePath, O_CREAT | O_WRONLY | O_TRUNC, VAULT_FILE_PERMISSIONS)
    time_t now;
    int res = getCurrentTime(&now);
    if (res != 0) return res;
    Catalog catalog1;
    catalog1.repoMD.creationTimestamp = now;
    catalog1.repoMD.lastModified = now;
    catalog1.repoMD.fileSize = fileSize;
    catalog1.repoMD.numberOfFiles = 0;
    catalog1.repoMD.numberOfGaps = 1;
    Block mainGap = catalog1.gapArray[0];
    mainGap.offset = sizeof(Catalog);
    mainGap.size = catalog1.repoMD.fileSize - sizeof(Catalog);
    catalog1.gapArray[0] = mainGap;
    WRITE_CATALOG(vfd)
    lseek(vfd, fileSize - 1, SEEK_SET);
    WRITE(vfd, "\0", sizeof(char))
    close(vfd);
    printf("Result: A vault created\n");
    return 0;
}

int list(char *vaultFilePath) {
    int vfd;
    OPEN(vfd, vaultFilePath, O_RDONLY, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd, vaultFilePath)
    int i = 0;
    char *printableInsertionTime;
    for (; i < catalog1.repoMD.numberOfFiles; i++) {
        FAT currentFAT = catalog1.FATArray[i];
        if ((printableInsertionTime = convertToPrintableDate(currentFAT.insertionDate)) == NULL)
	        return 1;
        printf("%s %zuB 0%3o %s\n", currentFAT.fileName, currentFAT.fileSize,
               currentFAT.fileProtection, printableInsertionTime);
        free(printableInsertionTime);
    }
    close(vfd);
    return 0;
}

void findGap(Catalog catalog1, int *gapsIndexes, ssize_t *maximum, int *maximumIndex){
    int j = 0;
    for (; j < catalog1.repoMD.numberOfGaps - 1; j++) {
        if (catalog1.gapArray[j].size > PREFIX_AND_SUFFIX_LENGTH && j != gapsIndexes[0] &&
            j != gapsIndexes[1] && catalog1.gapArray[j].size > *maximum) {
            *maximum = catalog1.gapArray[j].size;
            *maximumIndex = j;
        }
    }
}

int findRelevantGaps(Catalog catalog1, ssize_t fileToInsertSize, int *gapsIndexes) {
    ssize_t maximum;
    int maximumIndex;
    int i = 0;
    ssize_t leftToPlace = fileToInsertSize;
    for (; i < NUM_OF_FRAGMENTS; i++) {
        leftToPlace += PREFIX_AND_SUFFIX_LENGTH;
        maximum = 0;
        maximumIndex = -1;
        findGap(catalog1, gapsIndexes, &maximum, &maximumIndex);
        if (i == NUM_OF_FRAGMENTS - 1){
            if (maximum < leftToPlace &&
                    catalog1.gapArray[catalog1.repoMD.numberOfGaps - 1].size >= leftToPlace) {
                maximum = catalog1.gapArray[catalog1.repoMD.numberOfGaps - 1].size;
                maximumIndex = catalog1.repoMD.numberOfGaps - 1;
            }
        }

        gapsIndexes[i] = maximumIndex;
        if (maximum >= leftToPlace) return 0;
        if (!maximum) leftToPlace -= PREFIX_AND_SUFFIX_LENGTH;
        leftToPlace -= maximum;
    }

    return 1;
}

int writeChunks(int fd1, int fd2, ssize_t sizeToWrite) {
    ssize_t currentBytes = 0;
    char *buffer;
    size_t bufferSize = sizeToWrite >= MEGABYTE ? LARGE_BUFFER_SIZE : SMALL_BUFFER_SIZE;
    if ((buffer = (char *) malloc(bufferSize)) == NULL){
        printf("Couldn't allocate memory for the buffer");
        return 1;
    }
    while (currentBytes < sizeToWrite) {
        bufferSize = (sizeToWrite - currentBytes) < bufferSize ? (size_t) (sizeToWrite - currentBytes) : bufferSize;
        READ(fd2, buffer, bufferSize)
        WRITE(fd1, buffer, (size_t) readSize)
        currentBytes += readSize;
    }
    free(buffer);
    return 0;
}

int writeIntoGap(int vfd, int ifd, Catalog *catalog1, FAT *currentFAT, int currentFATIndex,
                 int currentFragmentIndex, int gapIndex) {
    ssize_t totalSize = currentFAT->fileSize;
    ssize_t leftToPlace = totalSize;
    int i = 0;
    for (; i < currentFragmentIndex; i++)
        leftToPlace -= (currentFAT->fragments[i].size - PREFIX_AND_SUFFIX_LENGTH);
    Block currentGap = catalog1->gapArray[gapIndex];
    ssize_t toWrite = (currentGap.size - PREFIX_AND_SUFFIX_LENGTH) < leftToPlace
                      ? (currentGap.size - PREFIX_AND_SUFFIX_LENGTH) : leftToPlace;
    lseek(vfd, currentGap.offset + PREFIX_LENGTH, SEEK_SET);
    lseek(ifd, currentFAT->fileSize - leftToPlace, SEEK_SET);
    int res = writeChunks(vfd, ifd, toWrite);
    if (res != 0) return res;
    lseek(vfd, currentGap.offset, SEEK_SET);
    WRITE(vfd, PREFIX_DELIMITER, PREFIX_LENGTH)
    lseek(vfd, currentGap.offset + PREFIX_LENGTH + toWrite, SEEK_SET);
    WRITE(vfd, SUFFIX_DELIMITER, SUFFIX_LENGTH)
    ssize_t fragmentSize = toWrite + PREFIX_AND_SUFFIX_LENGTH;
    currentFAT->fragments[currentFragmentIndex].size = fragmentSize;
    currentFAT->fragments[currentFragmentIndex].offset = currentGap.offset;
    currentFAT->numberOfFragments += 1;
    currentGap.size -= fragmentSize;
    currentGap.offset += fragmentSize;
    catalog1->FATArray[currentFATIndex] = *currentFAT;
    catalog1->gapArray[gapIndex] = currentGap;
    return 0;
}

void cleanGaps(Catalog *catalog1) {
    int i = 0, j = 0, n = 0;
    short k = 0;
    for (; i < catalog1->repoMD.numberOfGaps - k; i++) {
        if (catalog1->gapArray[n].size <= 0){
            for (j = n; j < catalog1->repoMD.numberOfGaps - 1 - k; j++)
                catalog1->gapArray[j] = catalog1->gapArray[j + 1];
            k++;
        }
        else n++;
    }
    catalog1->repoMD.numberOfGaps-=k;
}

void unionGaps(Catalog *catalog1) {
    int i = 0;
    while (i < catalog1->repoMD.numberOfGaps - 1) {
        Block currentGap = catalog1->gapArray[i];
        Block nextGap = catalog1->gapArray[i + 1];
        if (currentGap.offset + currentGap.size == nextGap.offset) {
            currentGap.size += nextGap.size;
            nextGap.size = -1;
            catalog1->gapArray[i] = currentGap;
            catalog1->gapArray[i + 1] = nextGap;
            cleanGaps(catalog1);
        } else
            i++;
    }
}

void insertGap(Catalog *catalog1, Block gap) {
    int i = 0, j;
    for (; i < catalog1->repoMD.numberOfGaps; i++)
        if (catalog1->gapArray[i].offset > gap.offset) break;
    for (j = catalog1->repoMD.numberOfGaps - 1; j >= i ; j--)
        catalog1->gapArray[j + 1] = catalog1->gapArray[j];
    catalog1->gapArray[i] = gap;
    catalog1->repoMD.numberOfGaps++;
}

void removeFragments(FAT *currentFAT, int fragmentIndex){
    int i = 0;
    for (; i < currentFAT->numberOfFragments; i++)
        if (i == fragmentIndex) break;
    for (; i < currentFAT->numberOfFragments - 1; i++)
        currentFAT->fragments[i] = currentFAT->fragments[i + 1];
    currentFAT->fragments[i].size = 0;
    currentFAT->fragments[i].offset = 0;
    currentFAT->numberOfFragments -= 1;
}

int turnFragmentIntoGap(int vfd, Catalog *catalog1, int FATIndex, int fragmentIndex){
    FAT currentFAT = catalog1->FATArray[FATIndex];
    Block currentFragment = currentFAT.fragments[fragmentIndex];
    lseek(vfd, currentFragment.offset, SEEK_SET);
    WRITE(vfd, DELETION_MARKER, PREFIX_LENGTH)
    lseek(vfd, currentFragment.offset + currentFragment.size - SUFFIX_LENGTH, SEEK_SET);
    WRITE(vfd, DELETION_MARKER, SUFFIX_LENGTH)
    insertGap(catalog1, currentFragment);
    removeFragments(&currentFAT, fragmentIndex);
    catalog1->FATArray[FATIndex] = currentFAT;
    return 0;
}

void turnFragmentsIntoGaps(int vfd, Catalog *catalog1, int FATIndex) {
    int i = 0;
    FAT currentFAT = catalog1->FATArray[FATIndex];
    for (; i < currentFAT.numberOfFragments; i++) {
        turnFragmentIntoGap(vfd, catalog1, FATIndex, i);
    }
}

int fileNameExists(Catalog catalog1, char *fileName) {
    int i = 0;
    for (; i < catalog1.repoMD.numberOfFiles; i++) {
        if (strcmp(fileName, catalog1.FATArray[i].fileName) == 0)
            return i;
    }
    return -1;
}

int insert(char *vaultFilePath, char *toInsertFilePath) {
    int vfd, ifd;
    OPEN(vfd, vaultFilePath, O_RDWR, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd, vaultFilePath);
    if (catalog1.repoMD.numberOfFiles >= 100){
        printf("There are already %d files\n", NUM_OF_FILES);
        close(vfd);
        return 1;
    }
    char *fileName = basename(toInsertFilePath);
    VALIDATE_NAME(catalog1, fileName, true)
    struct stat *stat1;
    if ((stat1 = (struct stat *) malloc(sizeof(struct stat))) == NULL) {
        close(vfd);
        printf("Couldn't allocate memory for the stats");
        return 1;
    }
    int res = getStats(toInsertFilePath, stat1);
    if (res != 0) {
        free(stat1);
        close(vfd);
        printf("Couldn't get stats for the file: %s\n", toInsertFilePath);
        return res;
    }
    int gapsIndexes[] = {-1, -1, -1};
    int i = 0, numberOfGaps = 0;
    res = findRelevantGaps(catalog1, stat1->st_size, gapsIndexes);
    for (; i < NUM_OF_FRAGMENTS; i++)
        if (gapsIndexes[i] != -1) numberOfGaps++;
    if (res != 0) {
        ssize_t freeSpace = getFreeSpace(catalog1);
        if (stat1->st_size + numberOfGaps * PREFIX_AND_SUFFIX_LENGTH > freeSpace)
            printf("Not enough space. Required: %zu, Free: %zu\n", stat1->st_size, freeSpace);
        else
            printf("Can't split the files into at most 3 fragments\n");
        free(stat1);
        close(vfd);
        return res;
    }
    int currentFATIndex = catalog1.repoMD.numberOfFiles;
    FAT currentFAT = catalog1.FATArray[currentFATIndex];
    currentFAT.numberOfFragments = 0;
    currentFAT.fileSize = stat1->st_size;
    time_t time1;
    res = getCurrentTime(&time1);
    if (res != 0) return res;
    currentFAT.insertionDate = time1;
    currentFAT.fileProtection = stat1->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    strcpy(currentFAT.fileName, fileName);
    OPEN(ifd, toInsertFilePath, O_RDONLY, stat1->st_mode)
    int j = 0;
    for (i = 0; i < NUM_OF_FRAGMENTS; i++) {
        if (gapsIndexes[i] != -1) {
            res = writeIntoGap(vfd, ifd, &catalog1, &currentFAT, currentFATIndex, j, gapsIndexes[i]);
            if (res != 0)
		        return res;
            j++;
        }
    }
    cleanGaps(&catalog1);
    catalog1.repoMD.numberOfFiles++;
    res = getCurrentTime(&time1);
    if (res != 0) return res;
    catalog1.repoMD.lastModified = time1;
    WRITE_CATALOG(vfd)
    close(vfd);
    close(ifd);
    printf("Result: %s inserted\n", fileName);
    return 0;
}

void removeFATFromCatalog(Catalog *catalog1, int FATIndex) {
    int i;
    for (i = FATIndex; i < catalog1->repoMD.numberOfFiles - 1; i++)
        catalog1->FATArray[i] = catalog1->FATArray[i + 1];
    catalog1->repoMD.numberOfFiles -= 1;
}

void deleteFAT(int vfd, Catalog *catalog1, int FATIndex){
    turnFragmentsIntoGaps(vfd, catalog1, FATIndex);
    unionGaps(catalog1);
    removeFATFromCatalog(catalog1, FATIndex);
}

int delete(char *vaultFilePath, char *fileName) {
    int vfd;
    OPEN(vfd, vaultFilePath, O_RDWR, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd, vaultFilePath);
    VALIDATE_NAME(catalog1, fileName, false)
    int FATIndex = fileNameExists(catalog1, fileName);
    deleteFAT(vfd, &catalog1, FATIndex);
    time_t time1;
    int res = getCurrentTime(&time1);
    if (res != 0) return res;
    catalog1.repoMD.lastModified = time1;
    WRITE_CATALOG(vfd)
    close(vfd);
    printf("Result: %s deleted\n", fileName);
    return 0;
}

int writeFileIntoCurrentDirectory(int vfd, FAT FAT1) {
    int fd = open(FAT1.fileName, O_CREAT | O_WRONLY | O_TRUNC, FAT1.fileProtection);
    int i = 0;
    for (; i < FAT1.numberOfFragments; i++) {
        lseek(vfd, FAT1.fragments[i].offset + PREFIX_LENGTH, SEEK_SET);
        int res = writeChunks(fd, vfd, FAT1.fragments[i].size - PREFIX_AND_SUFFIX_LENGTH);
        if (res != 0) return res;
    }
    return 0;
}

int fetch(char *vaultFilePath, char *fileName) {
    int vfd;
    OPEN(vfd, vaultFilePath, O_RDONLY, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd, vaultFilePath);
    VALIDATE_NAME(catalog1, fileName, false)
    int FATIndex = fileNameExists(catalog1, fileName);
    FAT FAT1 = catalog1.FATArray[FATIndex];
    int res = writeFileIntoCurrentDirectory(vfd, FAT1);
    if (res != 0) return res;
    printf("Result: %s created\n", fileName);
    return 0;
}

void sortFragmentsByOffset(Block **fragments, int numOfFragments){
    int i = 0, j;
    Block *tmpFragment;
    for (; i < numOfFragments; i++)
        for (j = i + 1; j < numOfFragments; j++)
            if (fragments[i]->offset > fragments[j]->offset) {
                tmpFragment = fragments[i];
                fragments[i] = fragments[j];
                fragments[j] = tmpFragment;
            }
}

int indentFragmentKSteps(int vfd1, int vfd2, Block *fragment, ssize_t k){
    lseek(vfd1, fragment->offset - k, SEEK_SET);
    lseek(vfd2, fragment->offset, SEEK_SET);
    int res = writeChunks(vfd1, vfd2, fragment->size);
    if (res != 0) return res;
    size_t suffixLength = k < SUFFIX_LENGTH ? (size_t)k : SUFFIX_LENGTH;
    lseek(vfd1, fragment->offset + fragment->size - suffixLength, SEEK_SET);
    WRITE(vfd1, DELETION_MARKER, SUFFIX_LENGTH)
    size_t prefixLength = fragment->size - (size_t)k;
    prefixLength = prefixLength > 0 ? prefixLength : PREFIX_LENGTH;
    prefixLength = prefixLength >= PREFIX_LENGTH ? 0 : prefixLength;
    lseek(vfd1, fragment->offset, SEEK_SET);
    WRITE(vfd1, DELETION_MARKER, prefixLength)

    fragment->offset -= k;
    return 0;
}

int handleFragmentsBetweenTwoGaps(int vfd1, int vfd2, Block gap1, Block gap2, Block **sortedFragments, int numberOfFragments) {
    int i = 0;
    Block *currentFragment;
    for (; i < numberOfFragments; i++) {
        currentFragment = sortedFragments[i];
        if (currentFragment->offset > gap1.offset) break;
    }
    for (; i < numberOfFragments; i++) {
        currentFragment = sortedFragments[i];
        if (currentFragment->offset > gap2.offset) break;
        int res = indentFragmentKSteps(vfd1, vfd2, currentFragment, gap1.size);
        if (res != 0) return res;
    }
    return 0;
}

int defrag(char *vaultFilePath) {
    int vfd1, vfd2;
    OPEN(vfd1, vaultFilePath, O_RDWR, VAULT_FILE_PERMISSIONS)
    OPEN(vfd2, vaultFilePath, O_RDONLY, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd2, vaultFilePath)

    int i = 0, j, numberOfFragments = 0;
    FAT currentFAT;
    Block *fragments[NUM_OF_FILES * NUM_OF_FRAGMENTS];
    for (; i < catalog1.repoMD.numberOfFiles; i++){
        currentFAT = catalog1.FATArray[i];
        for (j = 0; j < currentFAT.numberOfFragments; j++){
            fragments[numberOfFragments] = &catalog1.FATArray[i].fragments[j];
            numberOfFragments++;
        }
    }
    sortFragmentsByOffset(fragments, numberOfFragments);
    while(catalog1.repoMD.numberOfGaps > 1) {
        Block currentGap = catalog1.gapArray[0];
        Block nextGap = catalog1.gapArray[1];
        handleFragmentsBetweenTwoGaps(vfd1, vfd2, currentGap, nextGap, fragments, numberOfFragments);
        currentGap.offset = nextGap.offset - currentGap.size;
        catalog1.gapArray[0] = currentGap;
        unionGaps(&catalog1);
    }
    time_t time1;
    int res = getCurrentTime(&time1);
    if (res != 0) return res;
    catalog1.repoMD.lastModified = time1;
    WRITE_CATALOG(vfd1)
    close(vfd1);
    close(vfd2);
    printf("Result: Defragmentation complete\n");
    return 0;
}

bool checkIfGapIsBeforeAnyFile(Catalog catalog1, Block gap){
    int i = 0, j;
    for (; i < catalog1.repoMD.numberOfFiles; i++){
        for (j = 0; j < catalog1.FATArray[i].numberOfFragments; j++)
            if (catalog1.FATArray[i].fragments[j].offset < gap.offset) return false;
    }
    return true;
}

int status(char *vaultFilePath){
    int vfd;
    OPEN(vfd, vaultFilePath, O_RDWR, VAULT_FILE_PERMISSIONS)
    READ_CATALOG(vfd, vaultFilePath)
    ssize_t fragmentsSize = 0;
    ssize_t filesSize = 0;
    ssize_t gapsSize = 0;
    int i = 0, j;
    for (; i < catalog1.repoMD.numberOfFiles; i++) {
        filesSize += catalog1.FATArray[i].fileSize;
        for (j = 0; j < catalog1.FATArray[i].numberOfFragments; j++)
            fragmentsSize += catalog1.FATArray[i].fragments[j].size;
    }
    for (i = 0; i < catalog1.repoMD.numberOfGaps - 1; i++) {
        if (i == 0 && checkIfGapIsBeforeAnyFile(catalog1, catalog1.gapArray[i])) continue;
        gapsSize += catalog1.gapArray[i].size;
    }
    double fragmentationRatio = 0;
    if ((fragmentsSize + gapsSize) > 0)
        fragmentationRatio = (double)gapsSize / (fragmentsSize + gapsSize);
    printf("Number of files: %d\n"
           "Total size: %zuB\n"
           "Fragmentation ratio: %f\n", catalog1.repoMD.numberOfFiles,
           filesSize, fragmentationRatio);
    return 0;
}

void printElapse(struct timeval start, struct timeval end){
    long seconds, useconds;
    double mtime;
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    mtime = ((seconds) * 1000 + useconds / 1000.0);
    printf("Elapsed time: %.3f milliseconds\n", mtime);
}

int main(int argc, char *argv[]) {
    struct timeval start, end;
    int res = 0;
    gettimeofday(&start, NULL);
    if (!strcmp(argv[2], "init"))
        res = init(argv[1], parseDataAmountIntoInteger(argv[3]));
    else if (!strcmp(argv[2], "list"))
        res = list(argv[1]);
    else if (!strcmp(argv[2], "add"))
        res = insert(argv[1], argv[3]);
    else if (!strcmp(argv[2], "rm"))
        res = delete(argv[1], argv[3]);
    else if (!strcmp(argv[2], "fetch"))
        res = fetch(argv[1], argv[3]);
    else if (!strcmp(argv[2], "defrag"))
        res = defrag(argv[1]);
    else if (!strcmp(argv[2], "status"))
        res = status(argv[1]);
    gettimeofday(&end, NULL);
    printElapse(start, end);
    return res;
}
