# AWS SageMaker Edge (Version 1.20210304) Release Notes:
Amazon SageMaker Edge Manager optimizes models to run faster on edge devices.
It also helps manage models deployed on these devices. With SageMaker Edge Manager,
you can optimize, run, monitor, and update machine learning models across fleets of devices at the edge.

There are two major components of this service: (i) software for the Edge
device where optimized inference is run (ii) cloud component to monitor ML models across
fleets of devices.

This is an updated release of software for the Edge device (also referred
to as the Agent).

## Highlights (Changelog)
- Added ability to periodically upload capture data files to cloud using Provider API, in a configuration where capture data files are written to Disk.
- Added a new API to Provider APIs which allows for definition of "network status" by the provider implementation.
- [BUGFIX] Agent now allows for inference requests to proceed even when the device is not connected to the network.
- Configuration allows the user to toggle verbose logging in the agent via the config: `sagemaker_edge_log_verbose = true (false)`.
- With this release, the model name in the console dashboard will be the same as model name supplied at the edge packaging job stage.
- Added instructions to install the agent as a `systemd` service. The recommended steps are documented in `INSTALL.md`.

## Platform Support

### Linux:

-   Supported Ubuntu18.04 or later
-   Also works on systems running older glibc (versions 2.17 or later)

****Format:**** x86-64 bit (ELF binary) and ARMv8 64 bit (ELF binary)

### Windows:

-   Windows 10 version 1909

****Format:**** x86-32 bit (DLL) and x86-64 bit (DLL)

## Known Issues

-   Client applications should serialize calls to control plane APIs supported by gRPC,
    note that prediction requests are internally serialized by the inference engine.
-   Redirection of logs to a file is not supported yet,
    users are encouraged to redirect them to a file in their favorite shell/terminal
-   We recommend the users re-package their compiled models to benefit from the availability of model info in the artifact.
    -   The older binaries of agent will no longer work with newer re-packaged / compiled models.

## Documentation
https://docs.aws.amazon.com/sagemaker/latest/dg/edge.html

# AWS SageMaker Edge (Version 1.20201218) Release Notes:
Amazon SageMaker Edge Manager optimizes models to run faster on edge devices.
It also helps manage models deployed on these devices. With SageMaker Edge Manager,
you can optimize, run, monitor, and update machine learning models across fleets of devices at the edge.

There are two major components of this service: (i) software for the Edge
device where optimized inference is run (ii) cloud component to monitor ML models across
fleets of devices.

This is an updated release of software for the Edge device (also referred
to as the Agent).

## Highlights (Changelog)
- Resolved issue where prediction can crash when the device does not have internet access.
- Resolved issue on Windows where model metrics collected is empty.

## Platform Support

### Linux:

-   Supported Ubuntu18.04 or later
-   Also works on systems running older glibc (versions 2.17 or later)

****Format:**** x86-64 bit (ELF binary) and ARMv8 64 bit (ELF binary) 

### Windows:

-   Windows 10 version 1909

****Format:**** x86-32 bit (DLL) and x86-64 bit (DLL)

## Known Issues

-   Client applications should serialize calls to control plane APIs supported by gRPC,
    note that prediction requests are internally serialized by the inference engine.
-   Redirection of logs to a file is not supported yet,
    users are encouraged to redirect them to a file in their favorite shell/terminal

## Documentation
https://docs.aws.amazon.com/sagemaker/latest/dg/edge.html

# AWS SageMaker Edge (Version 1.20201208) Release Notes:

Amazon SageMaker Edge Manager optimizes models to run faster on edge devices.
It also helps manage models deployed on these devices. With SageMaker Edge Manager,
you can optimize, run, monitor, and update machine learning models across fleets of devices at the edge.

There are two major components of this service: (i) software for the Edge
device where optimized inference is run (ii) cloud component to monitor ML models across
fleets of devices.

This is the first release of software for the Edge devices (also referred
to as the Agent).


## Highlights

### Model Management

Provides gRPC interface to load, unload, list, and describe AWS SageMaker Edge Manager's packaged models.

### Optimized Inference

Provides an interface to efficiently run predictions on Edge devices.

### Model Metrics

Uploads model metrics periodically to the cloud for model monitoring.

### Capture Data

Allows users to capture input and output tensors along with auxiliary data.

## Platform Support

### Linux:

-   Supported Ubuntu18.04 or later
-   Also works on systems running older glibc (versions 2.17 or later)

****Format:**** x86-64 bit (ELF binary) and ARMv8 64 bit (ELF binary) 

### Windows:

-   Windows 10 version 1909

****Format:**** x86-32 bit (DLL) and x86-64 bit (DLL)

## Known Issues:

-   Model Metrics collected on Windows platform is empty
-   Client applications should serialize calls to control plane APIs supported by gRPC,
    note that prediction requests are internally serialized by the inference engine.
-   Redirection of logs to a file is not supported yet,
    users are encouraged to redirect them to a file in their favorite shell/terminal

## Best Practices:

This list is not comprehensive.

-   Many recent ML models can take up large amounts of disk space, in the order of GBs.
    Users are expected to monitor the disk utilization on their edge devices to ensure
    sufficient disk space (and other resources) are available on the Edge devices
    prior to installation or loading of the ML models.
-   Note that though gRPC/protobuf supports client applications implemented in multiple
    programming languages, internally SageMaker Edge APIs are implemented and tested in Modern C++.
-   Sanitize file permissions to all the local directories configured with SageMaker Edge.
    This includes the permissions for model package, socket file, shared memory segment path,
    data capture path on the disk. We recommend using Access Control security policies via SELinux,
    AppArmor, or other similar security modules supported by the OS of the Edge device.
-   Users are encouraged to launch the agent on Edge devices using a system/service manager such as `systemd`.
-   The following permission checks are enforced:
    -   The shared memory segment passed to the agent should be read-only (Linux: no users have write/exec permissions, Windows: N/A).
    -   The root certificates under the folder `sagemaker_edge_core_root_certs_path` defined in the config file should be read-only (Linux: no users have write/exec permissions, Windows: the read-only attribute must be set).

## Getting Started with AWS SageMaker Edge

-   You can get started with AWS SageMaker Edge Manager [here](<https://docs.aws.amazon.com/sagemaker/latest/dg/edge.html>)
