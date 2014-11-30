STE_pi
======

Sea Track Editor for OpenCPN
  This program reads both NMEA data stream files i.e. VDR (.txt), and Sea Trace (.trt) files from SeaClear II (with the Sea Trace function). When reading VDR files it creates a (.trt) file of the same name which is then re-read as input for the program. The produced file (.trt) can be used with Sea Track Edit program which is available elsewhere. There is additionally a Python program to directly make the NMEA (.txt) to STE (.trt) file conversion.

Operation:
  The initial file type must be set in the Preferences dialog along with the desired interval for wind barb spacing for the graphical display. Currently the choices are for .01 and .001 miles for either ocean or lake tracks.

  Upon loading the NMEA or TRT file the Track is displayed with wind barbs. The display window indicates the Track file name along with the initial start and end records with dates and times. The sliders allow changing of the start and end records to change the deired display and analysis area.

  Altered Tracks can be saved as a special TRT file with an "ext" name extension.

  The displayed Track is also available in the regular OpenCPN Route editor but is a TEMPORARY track and so not available after shutdown. Changes used with the OpenCPN Route editor will not be saved unless the track is seperately saved as an OpenCPN track which doesn't have the TRT funcionality.

USE:
  Historical data can be edited for desired sections which then are saved as STE (.trt) files. 
  STE (.trt) files can be used to generate Polar POL files using the Polar_pi program.
  The POL files can then be inserted into the TackandLay_pi program for use while sailing.

NOTE:
  When closing OpenCPN it's necessary to shut down the STE Plug-in first. This behaviour is unfortunate and will be resolved in the future.
