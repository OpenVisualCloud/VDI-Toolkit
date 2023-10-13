# MediaSample Guide
This is a sample to do screen capture and encode images in h264/h265 format with QES library. There are different configurations for different feature test.

## Build
Open ``MediaSample.sln`` and build the solution in default config. A binary file MediaSample.exe will appear in ``x64/Debug`` or ``x64/Release`` folder. You can also get the sample application with pre-built binary as follows.
There is a ``MediaSample.exe`` pre-built in the ``x64/release`` folder. And this sample requires Visual C++ 2015+ runtime & ``QESlib.dll``, which could be built using QES sample. There is also a pre-built ``QESlib.dll`` placed in ``x64/Debug``, ``x64/Release`` and ``MediaSample/deps/bin`` folder.

## Usage:
```
MediaSample.exe -cfg [config_file] (-n [num_frames]) (-fixFPS [target_FPS]) (-encodeonly [0 or 1])
```
- -n: [num_frames] to be the amount of frames to be captured.

- -cfg: [config_file] is the QES config file. There is an ``config264.json`` and ``config265.json`` as a sample.

- -fixFPS: [target_FPS] shall be the target frame rate. If this parameter is not used, the sample will not set a maximum framerate.

- -encodeonly: set this to 1 to make the system capture several frames in advance and then encode these frames repeatedly. This will show a maximum performance of encoding. If this param is set to 0 or not used, the sample will keep capture the screen until the session ends.

## Example Command
```
MediaSample.exe -cfg config264.json -n 10000 -fixFPS 30 -encodeonly 1
```