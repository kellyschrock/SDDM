SDDM
====

SDDM is a Linux-hosted sample player, implemented as a Jack client, and triggered by MIDI inputs. It's designed to pre-load sample data and play it back in response to MIDI inputs, as fast as required. SDDM's object model is similar to that of a typical drum machine; The top-level object is a drum kit, with a collection of instruments mapped to MIDI note numbers. Each instrument contains a collection of "layers", each mapped to a velocity range. Each layer contains a "sample", containing stereo waveform data loaded from wave files on disk. Within a drumkit, you can you can define the instruments, their names, their note mappings, their layers and associated velocity ranges, pitch, pan, and level.  

In addition, you can put instruments in their own submixes, so they appear as separate channels to Jack and ALSA. This is useful if, for example, you want to record cymbals on one set of tracks, toms on another, and kick and snare on their own channels.  

SDDM was designed to work with high-res samples of individual drums. The ns7 kits (naturalstudio.co.uk) work very well. There are lots of others available at various locations on the web.


## New:
New: a UI SDDM now has a GUI (it was originally a command line app). The UI is a work in progress, and not all of the desired features are there, but at least you can interact with the samples from a user interface now, instead of editing an XML file every time you want to hear something different.  

### UI Issues:

There is no facility for only playing specific submixes in a kit at the moment. I haven't figured out how I want that to work yet. There's no UI for selecting available ALSA audio and MIDI ports. I just use the drag/drop facilities in QJackCtl for that. Logging is somewhat experimental. The command-line version of SDDM logged everything via cout/cerr, which is mostly useless in a GUI application. I've been playing with the idea of redirecting the output formerly destined for stdout/stderr to a listener that can display log output in a UI element. That is in place, but is incomplete.
Most of the graphics on the mixer strips in the UI are lifted from the Hydrogen drum machine. At some point, I'll update those to something original, but for now, this should explain the Hydrogen-like elements on the mixer strips.
There is currently no way to add a channel strip in the UI. Essentially, you create a drumkit file with a text editor, and open it in SDDM. This whole area of the application will be fixed next, as soon as I have time to do it.

## Scene Support: 
SDDM has basic support for "scenes", sets of mixer settings that you can switch between on the fly.


