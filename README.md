# myShell

As the name suggests, this project is a command-line shell. This shell allows for multiple processes to be run simultaneously in the foreground and in the background. If you are ever tired of your laptop’s terminal feel free to download this one! Follow the setup instructions here.

**Here is a list of functionalities my shell can perform:**



* Basic commands
    * ls
    * exit
    * touch &lt;filename>
    * rm &lt;filename>
    * cd &lt;path>
    * echo &lt;string>
    * make
    * ./&lt;executable>
    * !history: print out each command in the history
    * #&lt;n>: print and executes the n-th command in history
    * !&lt;prefic>: print and executes the most recent command
    * Ps: print information about all currently executing processes
* Logical operators for commands
    * _&&_: And
    * _||_: Or
    * _;_: Separator
* Redirection commands
    * '>': Output the command into a file
    * '>>': Append the output of a command into a file
    * '<': Input the content of a file as the input args of a command.
* Background external commands: &lt;command>& \
Command ending with “&” will be run as a background process
* Signal commands
    * Ctrl+C: (SIGINT) kill the currently running foreground
    * kill &lt;pid>: (SIGKILL) kill a specifid process with pid
    * stop &lt;pid>: (SIGSTOP) stop of prcoess with pid
    * cont &lt;pid>: (SIGCONT) continue a process with pid
* History: ./shell -h &lt;filename>
    * Storing and displaying the history of commands executed across shell sessions
    * Clear history (currently implementing)
* Run commands from a script file: ./shell -f &lt;filename>

Video explanation of my shell: (in the process of making…)

**Implementation & more details**

I have implemented this project using C. 

** Setup instructions**

After forking and cloning this project, in a terminal navigate to the myShell folder. Type:



1. “make” to make the shell executable
2.  “./shell” to run this sell
3. “make clean” to delete all of the object files and the shell executable
