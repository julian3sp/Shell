# Unix Shell in C++

A lightweight Unix shell implementation in C++ with support for background processes, command execution using `execve`, job tracking, custom prompts, and system resource usage monitoring via `getrusage`.

## Features
- Supports executing Unix commands using `execve`
- Handles background processes (`&` for running jobs in the background)
- Tracks and manages background jobs with the `jobs` command
- Measures and displays system resource usage using `getrusage`
- Implements built-in commands:
  - `cd <directory>`: Changes the working directory
  - `exit`: Exits the shell, ensuring background jobs are handled
  - `jobs`: Lists background jobs
  - `set prompt <new_prompt>`: Changes the shell prompt

## Installation
Clone the repository and compile the shell:

```sh
$ git clone <repository-url>
$ cd <repository-folder>
$ g++ -o shell main.cpp
```

## Usage
Run the shell by executing:

```sh
$ ./shell
```

Then, enter commands as you would in a normal shell. Examples:

```sh
==> ls -l
==> sleep 5 &
[12345] sleep
==> jobs
[12345] sleep
```

To exit, use:

```sh
==> exit
```

## Background Job Handling
When a job is run in the background using `&`, the shell tracks its progress and notifies completion.

Example:

```sh
==> sleep 5 &
[12345] sleep
==> Job [12345] sleep Completed
```

## System Resource Monitoring
For each foreground process, the shell displays resource usage details like:
- User & system CPU time
- Context switches
- Page faults
- Maximum memory usage

Example output:

```sh
User CPU time used: 10 ms
System CPU time used: 5 ms
Elapsed wall-clock time: 20 ms
```
## Author  
- **Julian Espinal**  
- **julianrespinal24@gmail.com**  
- **[https://github.com/julian3sp/]** 

