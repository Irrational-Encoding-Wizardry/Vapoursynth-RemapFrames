Description
===========

Allows easy remapping of frames in a clip through the use of a text file or an input string.

Ported from the Avisynth plugin written by James D. Lin

- RemapFrames, RemapFramesSimple, and ReplaceFramesSimple provide general control over manipulation of frame indices in a clip. They can be used in cases where SelectEvery isn't suitable, such as when the desired frames don't follow a regular pattern.

- They are also efficient alternatives to long chains of FreezeFrame, DeleteFrame, or ApplyRange calls.
  
- RemapFramesSimple and ReplaceFramesSimple are less powerful than RemapFrames but use a much simpler, and more basic syntax.
  
- Remf, Remfs and Rfs are shortcuts for RemapFrames, RemapFramesSimple and ReplaceFramesSimple. It is recommended you use these as these are easier to use and remember.

- The filename and mappings parameters are optional; when none of them is specified, an empty string is assumed. 

RemapFrames
===========
**Usage**
::
    remap.RemapFrames(clip baseclip[, string filename, string mappings, clip sourceclip]) 
    remap.Remf(clip baseclip[, string filename, string mappings, clip sourceclip])
Parameters:
    *baseclip*
        Frames from sourceclip are mapped into baseclip.
    *filename*
        The path/name of the text file that specifies the new frame mappings. 
    *mappings*
        A string containing frame mappings. Has higher precedence than the mappings from the text file. If both the text file and the mappings string map a frame, the one from the mappings string is chosen..
    *sourceclip*
        The source clip used to supply the new, remapped frames.
        (Default: Same as baseclip.)


Each line in the text file or in the mappings string must have one of the following forms:


- a z

    Replaces frame a in baseClip with frame z from sourceClip.

- [a b] z

    Replaces all frames in the range [a b] of baseClip with frame z from sourceClip.

- [a b] [y z]

    Replaces all frames in the range [a b] of baseClip with frames in the range [y z] from the sourceClip. If the input and output ranges do not have equal sizes, frames will be duplicated or dropped evenly from [y z] to match the size of [a b]. If y > z, the order of the output frames is reversed.

- # comment

    A comment. Comments may appear anywhere on a line; all text from the start of the # character to the end of the line is ignored.
Sample data file:
::
    [0 9] [0 4]     # the first ten frames will be 0, 0, 1, 1, 2, 2, 3, 3, 4, 4
    10 5            # show frame 5 in frame 10's place
    [15 20] 6       # replace frames [15 20] with frame 6
    [25 30] [35 40] # replace frames [25 30] with frames [35 40]
    [50 60] [60 50] # reverse the order of frames 50..60
Within each line, all whitespace is ignored.

By default, all frames are mapped to themselves. If multiple lines remap the same frame, the last remapping overrides any previous ones.

The output clip always will have the same number of frames as the input clip.

RemapFramesSimple
=================
**Usage**
::
    remap.RemapFramesSimple(clip clip[, string filename, string mappings]) 
    remap.Remfs(clip clip[, string filename, string mappings])
Parameters:
    *baseclip*
        The name of the text file that specifies the new frame mappings.
    *filename*
        The path/name of the text file that specifies the new frame mappings.
    *mappings*
        Mappings alternatively may be given directly in a string. **Unlike RemapFrames and ReplaceFrames, filename and mappings cannot be used together. It is also an error to not specify both filename and mappings.**


RemapFramesSimple takes a text file or a mappings string consisting of a sequence of frame numbers. **The number of frame mappings determines the number of frames in the output clip.** For example:
::
     # Generate a clip containing only the first five frames.
     remap.Remfs(clip, mappings="0 1 2 3 4")
     
     ---------------------------------------------------------
     
     # Duplicate frame 20 five times.
     remap.Remfs(clip, mappings="20 20 20 20 20")
     
ReplaceFramesSimple
=================
**Usage**
::
    remap.ReplaceFramesSimple(clip baseclip, clip sourceclip[, string filename, string mappings]) 
    remap.Rfs(clip baseclip, clip sourceclip[, string filename, string mappings])
Parameters:
    *baseclip*
        Frames from sourceclip are mapped into baseclip.
    *sourceclip*
        The source clip used to supply the new, remapped frames.
    *filename*
        The path/name of the text file that specifies the new frame mappings.
    *mappings*
        A string containing frame mappings. Has higher precedence than the mappings from the text file. If both the text file and the mappings string map a frame, the one from the mappings string is chosen.


ReplaceFramesSimple takes a text file or a mappings string consisting of sequences or ranges of frame numbers to replace. For example:
::
      # Replaces frames 10..20, 25, and 30 from baseClip with the
      # corresponding frames from sourceClip.
      remap.Rfs(baseClip, sourceClip, mappings="[10 20] 25 30")
     
      
      -------------------------------------------------------
      
      
      # Inverse-telecine a clip and fix individual frames that still show
      # combing.
      import havsfunc as haf
      clip = core.vivtc.VFM(clip)
      clip = core.vivtc.VDecimate(clip)
      deinterlaced = haf.QTGMC(clip, Preset='Medium', TFF=True)
      # Replace frames 30, 40, 50 with their deinterlaced versions.
      clip = core.remap.Rfs(clip, deinterlaced, mappings="30 40 50")
