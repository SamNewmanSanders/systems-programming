#include <string>
#include <iostream>

#include <unistd.h> // For read() and write() syscall wrappers
#include <fcntl.h>  // For open()

// Forward declaration
bool copyToStdout(int inputFd, const char *sourceName);

// The first arguement is the length of the array argv
// The argv array contains pointers to characters, which are the first character
// of each string arguement
// The first arguement is always the name of the program

int main(int argc, char *argv[])
{   
    bool success = true;

    std::cout << "Running the samns-cat program\n\n";

    // Handle --help input
    if (argc > 1 && std::string(argv[1]) == "--help")
    {
        std::cout << "Usage: " << argv[0] << " [FILE]...\n"
                  << "Copy FILE(s) to int inputFd)standard output.\n"
                  << "With no FILE, read from standard input.\n"
                  << "Use '-' to read standard input at that position.\n";
        return 0;
    }

    // Handle special case where only input is standard input
    else if (argc == 1)
    {
        // No operands: copy stdin
        success = copyToStdout(STDIN_FILENO, "stdin");
    }

    // Now handle the "usual" cases
    else
    {
        // Now cycle through the arguements
        for (int index = 1; index < argc; ++index)
        {   
            int fd;
            // We don't want to accidentally close the stdin fd
            bool ownsFd = false;


            if (std::string(argv[index]) == "-")
            {
                fd = STDIN_FILENO;
            }

            else
            {
                // Open using file descriptor with read only flag
                fd = open(argv[index], O_RDONLY);
                ownsFd = true;

                // ALWAYS check for errors
                if (fd == -1)
                {
                    // perror reads errno and prints the system's description of the failure.
                    // argv[index] identifies which filename caused the error.
                    std::perror(argv[index]);
                    success = false;
                    continue;
                }
            }

            if (!copyToStdout(fd, argv[index]))
            {
                success = false;
            }

            if (ownsFd)
            {
                close(fd);
            }
        }
    }
    
    return success ? 0 : 1;
}

bool copyToStdout(int inputFd, const char *sourceName)
{
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    while (true)
    {
        // ssize_t (signed size_t) is a data type big enough to store
        // the number of bytes read/written or -1 on error.
        ssize_t bytesRead = read(inputFd, buffer, BUFFER_SIZE);

        // For a file with 8193 bytes, bytesRead will be 
        // 4096, 4096, 1, 0 (end of file)

        if (bytesRead == 0)
        {   
            return true;
        }   
        
        // May error due to write only 
        if (bytesRead == -1)
        {
            std::perror(sourceName);
            return false;
        }

        // Write may write less bytes then you ask for
        // Keep track of bytes written
        
        ssize_t totalWritten = 0;

        // Loop until we have written to stdout
        while (totalWritten < bytesRead)
        {
            // write() takes the output file descriptor, data pointer, and byte count.
            ssize_t bytesWritten = write(
            STDOUT_FILENO,                  // Usually 1
            buffer + totalWritten,          // Offset pointer past written bytes
            bytesRead - totalWritten        // Write the bytes still remaining
            );

            if (bytesWritten == -1)
            {
                std::perror(sourceName);
                return false;
            }
            
            totalWritten += bytesWritten;
        }
    }
}