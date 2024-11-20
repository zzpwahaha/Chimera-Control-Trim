import sys
from datetime import datetime

class Logger:
    def __init__(self, filename):
        self.terminal = sys.stdout
        self.logfile = open(filename, 'a')
    
    def _get_timestamp(self):
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def write(self, message):
        if message.strip():  # Ignore empty messages
            timestamped_message = f"{self._get_timestamp()} - {message}\n"
            self.terminal.write(timestamped_message)  # Write to terminal
            self.logfile.write(timestamped_message)    # Write to file
    
    def flush(self):
        # This method is required for Python 3 compatibility
        self.terminal.flush()
        self.logfile.flush()

    def close(self):
        self.logfile.close()

if __name__ == '__main__':

    # Usage example
    logger = Logger('./test/logfile_test.log')
    sys.stdout = logger

    print("This message will be logged to the file and displayed on the terminal.")
    print("This message will be logged to the file and displayed on the terminal.")

    # Restore sys.stdout if needed
    sys.stdout = logger.terminal
    print("This message will only be displayed on the terminal.")

    # Clean up
    logger.close()
