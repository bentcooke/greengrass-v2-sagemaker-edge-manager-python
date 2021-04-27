# SageMaker Edge Manager Install Instruction

SageMaker Edge Manager agent can be run as a standalone process in the form of an Executable and Linkable Format (ELF) executable binary or can be linked against as a Dynamic Shared Object (.dll). Running as a standalone executable binary is the preferred mode and is supported on Linux. Running as a shared object (.dll) is supported on Windows. On Linux, we recommend that you run the binary via a service that’s a part of your initialization (init) system.

## Option 1: Install as system daemon

### Step 1: Download edge agent and extract its content
After downloading the .tgz, execute the following command to extract. Let's say downloading version `1.20201207.02d0e97`:
```
tar -xvf 1.20201207.02d0e97.tgz`
```

Then go to the extracted directory
```
cd 1.20201207.02d0e97
```

The directory would look like as below:
```
1.20201207.02d0e97/
| 
 -- ATTRIBUTIONS.txt
 -- bin/
 -- docs/
 -- LICENSE.txt
 -- install.sh
 -- agent.env
 -- sagemaker_edge_agent.service
 -- RELEASE_NOTES.md
 -- INSTALL.md
```

### Step 2: add socket address and config file path
The `agent.env` file has two arguments to fill in, in order to launch edge agent as systemd.
```
SOCKET_ADDRESS=<unix socket address>
AGENT_CONFIG_FILE_PATH=<path to agent configuration file>
```

An example would look like as:
```
SOCKET_ADDRESS=/tmp/sagemaker_edge_agent_example.sock
AGENT_CONFIG_FILE_PATH=/home/ubuntu/distribution/resources/neo_config.json
```

### Step 3: install edge agent
The installation script has been provided to user, user can directly utitlize the `install.sh` script to install and launch edge agent
```
sudo ./install.sh
```
The script will ask user to provide a directory to install SageMaker Edge Manager:
```
Enter a directory to install [press ENTER use default: /opt/aws/sagemaker_edge]:
```

#### Additional Steps:
`install.sh` will enable edge agent as a systemd service and run it. System daemon can also be kicked off manually:
```
systemctl start sagemaker_edge_agent.service
```

If you want to stop edge agent from running you could do:
```
systemctl stop sagemaker_edge_agent.service
```

Agent log can be found at: `/var/log/syslog`. You can also check it status by:
```
systemctl status sagemaker_edge_agent.service
```

A complete `systemctl` manual could be found at: https://man7.org/linux/man-pages/man1/systemctl.1.html

## Option 2: Use as binary executable 

If you want to run the binary directly, you can do so in a terminal as shown below. If you have a modern OS, there are no other installations necessary prior to running the agent, since all the requirements are statically built into the executable. This gives you flexibility to run the agent on the terminal, as a service, or within a container.

```
./sagemaker_edge_agent -a <ADDRESS_TO_SOCKET> -c <PATH_TO_CONFIG_FILE>
```
