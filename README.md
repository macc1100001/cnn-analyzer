# CNN-Analyzer
A simple CNN feature map visualizer
![demo](screenshots/demo.png)

Compile instructions

1. Remove **$(EXE)** from **all** in **lib/Darknet/Makefile** to only compile **.o** files. Like this

	Old

		`all: $(OBJDIR) backup results setchmod $(EXEC) $(LIBNAMESO) $(APPNAMESO)`

	New

		`all: $(OBJDIR) backup results $(LIBNAMESO) $(APPNAMESO)`

2. Open a terminal in lib/Darknet directory

3. Do `make -j`

4. Return to **cnn_analyze** main directory

5. Do `make -j`

To run execute the **run.sh** script like this `sh run.sh`
