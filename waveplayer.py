import sph
# pip install simeplaudio
import simpleaudio
import wave

# pip install pydub
from pydub import AudioSegment

# pip install python-osc
from pythonosc import osc_message_builder, osc_message

class waveplayer(object):

    def __init__(self):
        self.volume = 0.5
        self.pitch = 1
        
    def handleMsg(self, msg, type, name, uuid, *args, **kwargs):
    
        # Pop first frame as bytes, assumes single-frame message
        frameBytes = msg.pop()
        oscMsg = osc_message.OscMessage(frameBytes)
        
        if len(oscMsg.params) == 2:
            # Assume midi message
            channel = int(oscMsg.params[0])
            value = int(oscMsg.params[1])
            
            if channel == 1:
                # volume
                self.volume = ( ( 1 - ( value / 127.0 ) ) * 60 - 0.5 )
            elif channel == 11:
                # pitch
                self.pitch = ( value / 127 ) * 2 - 1
        else:
            # Assume play cue
            name = oscMsg.address[1:]
            
            # alter pitch with pydub
            sound = AudioSegment.from_file(name+'.wav', format="wav")
            sound = sound - self.volume
            octaves = self.pitch
            newSampleRate = sound.frame_rate * (2.0 ** octaves)
            hipitch_sound = sound._spawn(sound.raw_data, overrides={'frame_rate': int(newSampleRate)})
            hipitch_sound = hipitch_sound.set_frame_rate(44100)
            
            # sound buffer with simpleaudio (non-blocking)
            simpleaudio.play_buffer(
                hipitch_sound.raw_data,
                num_channels=hipitch_sound.channels,
                bytes_per_sample=hipitch_sound.sample_width,
                sample_rate=hipitch_sound.frame_rate
            )
        
        return None
