#!/usr/bin/env python3
"""
MGED Socket Debugging Tool

An interactive Python tool for testing the MGED Unix domain socket server interface.
Supports Unicode control character protocol with visual delimiter symbols.
"""

import sys
import socket
import argparse
import termios
import tty
import select
from typing import Optional

# ANSI color codes for output formatting
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

# MGED Protocol control characters
FS = '\x1c'  # File Separator (end of message)
GS = '\x1d'  # Group Separator (command/args separator)
RS = '\x1e'  # Record Separator (argument separator)

# Printable Unicode equivalents for display
FS_VISIBLE = '\u241c'  # ␜
GS_VISIBLE = '\u241d'  # ␝
RS_VISIBLE = '\u241e'  # ␞

def get_help_text() -> str:
    """Generate help text for the MGED Socket Debug Tool."""
    help_text = f"""
{Colors.HEADER}MGED Socket Debug Tool - Commands{Colors.ENDC}

{Colors.OKCYAN}Special Characters:{Colors.ENDC}
  Ctrl+G  : Insert Group Separator {Colors.BOLD}␝{Colors.ENDC} (GS - command/arg separator)
  Ctrl+R  : Insert Record Separator {Colors.BOLD}␞{Colors.ENDC} (RS - argument separator)  
  Ctrl+F  : Insert File Separator {Colors.BOLD}␜{Colors.ENDC} (FS - message terminator)

{Colors.OKCYAN}Built-in Commands:{Colors.ENDC}
  {Colors.BOLD}help{Colors.ENDC}     : Show this help message
  {Colors.BOLD}quit{Colors.ENDC}     : Exit the program

{Colors.OKCYAN}Protocol:{Colors.ENDC}
  Commands are automatically terminated with FS (␜) if not present.
  Arguments are separated by RS (␞) and grouped with GS (␝).
  
{Colors.OKCYAN}Examples:{Colors.ENDC}
  {Colors.BOLD}ls{Colors.ENDC}                    : List database contents
  {Colors.BOLD}opendb␝my_database.g{Colors.ENDC} : Open existing database
  {Colors.BOLD}in␝sphere.s␞sph␞0␞0␞0␞5 {Colors.ENDC}: Create sphere
  
{Colors.OKCYAN}Shared Database Behavior:{Colors.ENDC}
  All client connections share the same database instance. Changes made
  via the socket interface are immediately visible in MGED's RECoL
  and UI, and vice versa. Command execution is atomic and
  serialized across all interfaces.
"""
    return help_text

def format_response(response: str) -> str:
    """Format the server response with color coding."""
    if not response:
        return f"{Colors.WARNING}Empty response{Colors.ENDC}"
    
    lines = response.split('\n')
    formatted_lines = []
    
    for line in lines:
        if line.startswith('OK␝'):
            # Success response
            parts = line.split('␝', 2)
            if len(parts) >= 2:
                formatted_lines.append(f"{Colors.OKGREEN}Status: OK{Colors.ENDC}")
                if parts[1]:
                    formatted_lines.append(f"{Colors.OKCYAN}Result:{Colors.ENDC} {parts[1]}")
        elif line.startswith('ERR␝'):
            # Error response
            parts = line.split('␝', 3)
            if len(parts) >= 3:
                formatted_lines.append(f"{Colors.FAIL}Status: ERROR{Colors.ENDC}")
                if parts[2]:
                    formatted_lines.append(f"{Colors.WARNING}Message:{Colors.ENDC} {parts[2]}")
        elif line.strip():
            # Regular output line
            formatted_lines.append(line)
    
    return '\n'.join(formatted_lines)

def connect_to_socket(socket_path: str) -> socket.socket:
    """Connect to the MGED Unix domain socket."""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(socket_path)
        print(f"{Colors.OKGREEN}Connected to MGED socket: {socket_path}{Colors.ENDC}")
        return sock
    except socket.error as e:
        print(f"{Colors.FAIL}Error connecting to socket {socket_path}: {e}{Colors.ENDC}")
        sys.exit(1)

def send_command(sock: socket.socket, command: str, debug_mode: bool = False) -> str:
    """Send a command to the MGED server and return the response."""
    try:
        # Convert visible delimiters to actual control characters
        command = command.replace(FS_VISIBLE, FS)
        command = command.replace(GS_VISIBLE, GS)
        command = command.replace(RS_VISIBLE, RS)
        
        # Automatically add FS terminator if not present
        if FS not in command:
            command += FS
        
        if debug_mode:
            print(f"{Colors.WARNING}DEBUG: Sending raw bytes: {command.encode('unicode_escape')}{Colors.ENDC}")
        
        sock.send(command.encode('utf-8'))
        
        # Receive response
        response_data = sock.recv(4096)
        response = response_data.decode('utf-8')
        
        if debug_mode:
            print(f"{Colors.WARNING}DEBUG: Received raw bytes: {response.encode('unicode_escape')}{Colors.ENDC}")
        
        return response
    except socket.error as e:
        print(f"{Colors.FAIL}Socket error during communication: {e}{Colors.ENDC}")
        print(f"{Colors.FAIL}Connection lost, exiting...{Colors.ENDC}")
        sys.exit(1)

def getch() -> str:
    """Get a single character from stdin, handling special keys."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        ch = sys.stdin.read(1)
        
        # Check for control sequences
        if ch == '\x1b':  # ESC
            # Read more characters to detect sequences
            if select.select([sys.stdin], [], [], 0.1)[0]:
                ch += sys.stdin.read(2)
        
        return ch
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

def main():
    """Main entry point for the MGED Socket Debug Tool."""
    parser = argparse.ArgumentParser(description='Interactive MGED Socket Debug Tool')
    parser.add_argument('socket_path', help='Path to the MAGED Unix domain socket')
    parser.add_argument('--debug', '-d', action='store_true', help='Show raw protocol bytes for debugging')
    args = parser.parse_args()
    
    # Connect to socket before starting UI
    sock = connect_to_socket(args.socket_path)
    
    print(f"{Colors.OKCYAN}MGED Socket Debug Tool{Colors.ENDC}")
    print(f"{Colors.OKCYAN}Type 'help' for commands or Ctrl+D to exit{Colors.ENDC}")
    print()
    
    try:
        while True:
            # Get user input with special character handling
            sys.stdout.write(f"{Colors.OKGREEN}> {Colors.ENDC}")
            sys.stdout.flush()
            
            # Set up terminal for character-by-character input
            fd = sys.stdin.fileno()
            old_settings = termios.tcgetattr(fd)
            tty.setraw(fd)
            
            try:
                command_line = ""
                while True:
                    char = getch()
                    
                    # Handle special keys
                    if char == '\x04':  # Ctrl+D (EOF)
                        print()
                        return
                    
                    if char == '\r' or char == '\n':  # Enter
                        print()
                        break
                    
                    if char == '\x07':  # Ctrl+G (GS)
                        command_line += GS_VISIBLE
                        sys.stdout.write(GS_VISIBLE)
                    elif char == '\x12':  # Ctrl+R (RS)
                        command_line += RS_VISIBLE
                        sys.stdout.write(RS_VISIBLE)
                    elif char == '\x06':  # Ctrl+F (FS)
                        command_line += FS_VISIBLE
                        sys.stdout.write(FS_VISIBLE)
                    elif char == '\x7f' or char == '\x08':  # Backspace
                        if command_line:
                            command_line = command_line[:-1]
                            sys.stdout.write('\b \b')
                    elif ord(char) >= 32:  # Printable character
                        command_line += char
                        sys.stdout.write(char)
                    
                    sys.stdout.flush()
            finally:
                termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
            
            # Process command
            command_line = command_line.strip()
            if not command_line:
                continue
            
            # Handle built-in commands
            if command_line.lower() == 'help':
                print(get_help_text())
                continue
            elif command_line.lower() == 'quit':
                print(f"{Colors.OKCYAN}Goodbye!{Colors.ENDC}")
                break
            
            # Visual separator
            print(f"{Colors.OKCYAN}--- Command ---{Colors.ENDC}")
            print(f"{Colors.BOLD}{command_line}{Colors.ENDC}")
            print(f"{Colors.OKCYAN}--- Response ---{Colors.ENDC}")
            
            # Send command and display response
            response = send_command(sock, command_line, args.debug)
            print(format_response(response))
            print()
            
    except KeyboardInterrupt:
        print(f"\n{Colors.OKCYAN}Interrupted, exiting...{Colors.ENDC}")
    finally:
        sock.close()

if __name__ == '__main__':
    main()