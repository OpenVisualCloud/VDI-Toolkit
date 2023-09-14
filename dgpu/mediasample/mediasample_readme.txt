Open MediaSample.sln and build the solution in default config. An binary file MediaSample.exe will appear in \x64\Release folder. Could follow the usage below to use this sample.
This sample requires Visual C++ 2015+ runtime & "QESlib.dll", QESlib.dll could be built using QES sample. There is also a pre-built dll placed in x64\Debug, x64\Release and MediaSample\deps\bin.

Sep 2023 Update: we could build the solution and fetch the dependencies automatically by running MediaSample-build.bat. The built file will be located in bin\ folder.
We could use the following instruction to run MediaSample.exe.

usage:
MediaSample.exe -cfg [config_file] (-n [num_frames]) (-fixFPS [target_FPS]) (-encodeonly [0 or 1])
	-n: [num_frames] to be the amount of frames to be captured.
	-cfg: [config_file] is the QES config file. There is an config264.json and config265.json as a sample.
	-fixFPS: [target_FPS] shall be the target frame rate. If this parameter is not used, the sample will not set a maximum framerate.
	-encodeonly: set this to 1 to make the system capture several frames in advance and then encode these frames repeatedly. This will show a maximum performance of encoding. If this para is set to 0 or not used, the sample will keep capture the screen until the session ends.

sample:
MediaSample.exe -cfg config264.json -n 10000 -fixFPS 30 -encodeonly 1