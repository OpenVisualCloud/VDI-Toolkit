Reference code for VDI(Virtual Desktop Infrastructure), including:
- VDI test scripts, test tools
- Deployment utils, auto config tools
- BKMs
- Reference applications

Dependencies for dGPU:
nlohmann's json is required to parse configuration files. In order to get this component, it is required to clone it from https://github.com/nlohmann/json.git and copy the \include\nlohmann folder to \dgpu\mediasample\MediaSample\deps\include so that it could be used by the media sample.