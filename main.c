#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>

static inline const size_t generic_buf_size() {
    return (size_t)256;
}

void newline() {
    printf("%c", '\n');
}

// Print newline for n number of times.
void newline_n(int n) {
    for (int i = 0; i < n; i++) {
        printf("%c", '\n');
    }
}

int count_digits(int n) {
    int count = 0;
    do {
        n /= 10;
        count++;
    } while (n != 0);
    return count;
}

// Returns 1 if a file has a binary file extension or 0 if not.
uint8_t file_is_binary(const char* filename) {
    if (!filename) {return 0;}
    const size_t ext_count = 18;
    const char* binary_extensions[] =       {".exe", ".bin", ".dll", ".class", ".dat",
                                            ".jpg", ".jpeg", ".png", ".gif",
                                            ".mp3", ".wav",
                                            ".mp4", ".avi", ".mov", ".webm",
                                            ".zip", ".7z", ".rar"};

    size_t anchor_init = 0;                       // Ensure anchor ptr is initialized
    size_t* anchor = &anchor_init;                // Anchor address to avoid dereferencing null
    size_t* dot_pos = NULL;                       // Pointer to check failure/assign index of '.' in filename

    for (size_t i = 0; i < strlen(filename); i++) {
        if (filename[i] == '.') {
            dot_pos = anchor;           // Set address before dereferencing
            *dot_pos = i;               // Remember position of '.'
            break;
        }
    }

    if (!dot_pos) {return 0;}
    const char* temp = filename + *dot_pos;     // Sets a temp pointer to where the '.' is located in filename, then passes it to loop to compare strings

    for (size_t i = 0; i < ext_count; i++) {
        if (strcmp(temp, binary_extensions[i]) == 0) {return 1;}
    }
    return 0;
}

// Replaces newline character with null terminator.
void annihilate_trailing_newline(char* buffer, const size_t buffer_size) {
    for (size_t i = 0; i < buffer_size; i++) {
        if (buffer[i] == '\0') {return;}
        if (buffer[i] == '\n') {buffer[i] = '\0';}
    }
}

// Returns the size of a file's stream.
const size_t get_file_size(FILE* file) {
    if (!file) {return 0;}
    if (fseek(file, 0, SEEK_END) != 0) {                // Get file size for fread()
        printf("\nFSEEK returned a non-zero value.");
        return 0;
    }
    const size_t size = (size_t)ftell(file);
    rewind(file);
    return size;
}

// Prints the name and size of a file given a filename. Note that it also opens/closes the file, so call this before any fopen() calls.
// Takes an optional uint8 pointer. pass NULL to ignore, or a valid ptr to indicate file_is_binary() has already been called.
void print_metadata(const char* filename, uint8_t* __OPT_BIN_FLAG) {
    if (!filename) {return;}
    const uint8_t flag_passed = (__OPT_BIN_FLAG != NULL) ? 1 : 0;
    const uint8_t is_binary = (flag_passed) ? *__OPT_BIN_FLAG : file_is_binary(filename);
    const char* text_or_bin = (is_binary) ? "Binary" : "Text";
    FILE* fp = (is_binary) ? fopen(filename, "rb") : fopen(filename, "r");
    const size_t size = get_file_size(fp);
    printf("\nFilename:\t\t[%s]\n", filename);
    printf("Size:\t\t\t[%zu] Bytes\n", size);
    printf("Type (Text/Bin):\t[%s]", text_or_bin);
    fclose(fp);
}

// Dynamically saves all files in "..\data\\" to index.ini.
// Returns pointer to index.ini, in read mode.
FILE* index_init() {
    const char* index_filename = "index.ini";
    FILE* index = fopen(index_filename, "w");
    if (!index) {return NULL;}
    const size_t buffer_size = generic_buf_size();
    char buffer[buffer_size];
    DIR* data_dir = opendir(".\\data\\");
    if (!data_dir) {
        printf("\nError Writing to index.ini\n");
        return NULL;
    }
    struct dirent* data_entry;
    int i = 0;
    while ((data_entry = readdir(data_dir)) != NULL) {
        if (strcmp(".", data_entry->d_name) == 0 || strcmp("..", data_entry->d_name) == 0) {continue;}  // Prevents "." and ".." from being written to buffer
        i++;                                                                            // i must increment after strcmp but before write operation
        snprintf(buffer, buffer_size, "%s", data_entry->d_name);                        // Copy filenames to buffer
        fprintf(index, "%d=%s\n", i, buffer);                                           // Write "<i>=<buffer>" to index.ini file
        memset(buffer, 0, buffer_size);
    }
    if (i == 0) {
        printf("Nothing in data directory.\n");
        return NULL;
    }
    closedir(data_dir);
    fclose(index);
    index = fopen(index_filename, "r");    // Reopen index.ini in read mode and return the pointer
    return index;
}

void print_file_contents(FILE* file) {
    if (!file) {
        printf("\nPassed NULL FILE* to file print function.");
        return;
    }
    const size_t size = get_file_size(file);
    char buffer[size];
    size_t bytes = fread(buffer, sizeof(char), size, file);
    buffer[bytes] = '\0';                                   // Set end to null terminator so it does not print unicode
    rewind(file);
    printf("%s", buffer);
}

void print_hex(FILE* file) {
    if (!file) {
        printf("\nPassed NULL FILE* to file print function.");
        return;
    }
    const size_t linebreak = 16;
    const size_t file_size = get_file_size(file);
    for (size_t i = 0, j = 1; i < file_size; i++, j++) {
        uint32_t c = fgetc(file);
        if (c == EOF) {return;}
        printf("%02X  ", c);
        if (j == linebreak) {printf("\n");j=0;}
    }
    rewind(file);
}

// Prints the contents of all files listed in index.ini
void print_index_all(FILE* index_ini, uint8_t __METADATA_FLAG) {
    if (!index_ini) {return;}
    const char* dir = "data/";
    const size_t buffer_size = generic_buf_size();
    char content_buffer[buffer_size];
    char filename_buffer[buffer_size];
    void (*callback)(FILE*);
    int i = 1;
    while (fgets(content_buffer, buffer_size, index_ini)) {
        if (content_buffer[0] == '\0') {break;}
        int digits = count_digits(i);
        size_t offset = digits + 1;

        strcpy(filename_buffer, dir);                       // Construct a string from "data/", then the filename, followed by a '\0'
        strcat(filename_buffer, (content_buffer + offset));
        annihilate_trailing_newline(filename_buffer, buffer_size);

        uint8_t is_binary = file_is_binary(filename_buffer);
        if (__METADATA_FLAG) {print_metadata(filename_buffer, &is_binary);}
        
        FILE* fp = (is_binary) ? fopen(filename_buffer, "rb") : fopen(filename_buffer, "r");
        newline();
        callback = (is_binary) ? &print_hex : &print_file_contents;
        callback(fp);
        fclose(fp);
        newline();
        memset(filename_buffer, 0, buffer_size);
        i++;
    }
}

// Prints contents of a file given an index number. 
// Prints only that specified file listed in "index.ini"
void print_index_n(FILE* index_ini, int n, uint8_t __METADATA_FLAG) {
    if (!index_ini) {return;}
    const char* dir = "data/";
    const size_t buffer_size = generic_buf_size();
    char content_buffer[buffer_size];
    char filename_buffer[buffer_size];
    FILE* data = NULL;
    void (*callback)(FILE*);
    int row = 1;
    while (fgets(content_buffer, buffer_size, index_ini)) {
        if (n == row) {
            const int digits = count_digits(row);
            const size_t offset = digits + 1;
            strcpy(filename_buffer, dir);
            strcat(filename_buffer, (content_buffer + offset));
            annihilate_trailing_newline(filename_buffer, buffer_size);
            uint8_t is_binary = file_is_binary(filename_buffer);
            if (__METADATA_FLAG) {print_metadata(filename_buffer, &is_binary);}
            data = (is_binary) ? fopen(filename_buffer, "rb") : fopen(filename_buffer, "r");
            callback = (is_binary) ? &print_hex : &print_file_contents;
            break;
        }
        row++;
    }
    if (!data) {
        printf("\nInput file not found. Index:\t[%d]", row);
        return;
    }
    newline();
    callback(data);
    fclose(data);
}

void print_program_desc() {
    printf("\nTextpreview is a Command-Line Interface program that displays a read-only view of text files into the terminal.\n\n");
}

void print_commands() {
    printf("Below is a list of commands you can pass as arguments:\n");
    printf("\t-index N\tWhere N is the number listed in 'index.ini' corresponding to a file. If passed, the program displays the contents of only the specified file.\n");
    printf("\t-file FN\tWhere FN is the filename of a specific file in 'data/...' If passed, the program displays only the contents of the specified file.\n");
    printf("\t-meta\tDisplays basic metadata of a file such as size and whether it is a Text or Binary file format. This command may be used in addition with the previous two commands.\n");
    printf("\t-help\tYou are here.\n");
}

int main(int argc, char* argv[]) {
    const size_t buffer_size = generic_buf_size();
    char input_buffer[buffer_size];
    FILE* const INDEX_INI = index_init();

    if (!INDEX_INI) {
        printf("\nNull index.ini\n");
        return 1;
    }


    // CLI Flags
    uint8_t PASSED_INDEX = 0;
    uint8_t PASSED_FILENAME = 0;
    uint8_t PASSED_METADATA = 0;
    uint8_t PASSED_HELP = 0;

    // Remembers inputs
    uint32_t PASSED_INDEX_VAL;
    size_t FILENAME_ARGP;


    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-index") == 0) {
            if (argv[i+1] == NULL)      {break;}
            if (argv[i+1][0] == '-')    {continue;}
            if (argv[i+1][0] == '0')    {continue;}
            PASSED_INDEX = 1;
            PASSED_INDEX_VAL = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-file") == 0) {
            if (argv[i+1] == NULL)      {break;}
            if (argv[i+1][0] == '-')    {continue;}
            PASSED_FILENAME = 1;
            FILENAME_ARGP = i+1;
        }
        if (strcmp(argv[i], "-meta") == 0) {PASSED_METADATA = 1;}
        if (strcmp(argv[i], "-help") == 0) {PASSED_HELP = 1;}
    }

    if (PASSED_INDEX) {
        print_index_n(INDEX_INI, PASSED_INDEX_VAL, PASSED_METADATA);
        rewind(INDEX_INI);
    }
    if (PASSED_FILENAME) {
        const char* dir = "data/";
        strcpy(input_buffer, dir);
        strcat(input_buffer, argv[FILENAME_ARGP]);
        uint8_t is_binary = file_is_binary(input_buffer);
        if (PASSED_METADATA) {print_metadata(input_buffer, &is_binary);}
        FILE* fp = (is_binary) ? fopen(input_buffer, "rb") : fopen(input_buffer, "r");
        void (*callback)(FILE*) = (is_binary) ? &print_hex : &print_file_contents;
        newline();
        callback(fp);
        rewind(fp);
        newline();
        fclose(fp);
        memset(input_buffer, 0, buffer_size);
    }
    if (PASSED_HELP) {
        print_program_desc();
        print_commands();
        return 0;
    }
    


    if (PASSED_INDEX || PASSED_FILENAME) {return 0;}    // Program will terminate early here if user only wants a specific file

    // If not terminated, the default behavior is to print all the contents of files listed in "index.ini"
    newline_n(2);
    print_index_all(INDEX_INI, PASSED_METADATA);
    
    return 0;
}