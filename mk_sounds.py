from glob import glob


from collections import Counter
import wave, array

import sys, re, pprint
from math import sqrt

class SoundMgr :
    name="Sounds"
    def __init__(self) :
        self.wavs={} # name : filename, data
        self.frozen=False

    def name_from_file(self,wavfile) : 
        nm = wavfile.rsplit('/',1)[1].split('.')[0]
        nm = ''.join(c for c in nm if c.isalnum() or c =='_')
        return nm

    def add(self,wavfile,wavname=None) :
        "wavename is empty means try to get one from filename"
        assert not self.frozen, "already frozen wavmgr"

        if wavname == None : 
            wavname=self.name_from_file(wavfile)

        assert (wavname not in self.wavs) or (self.wavs[wavname][0]==wavfile), "duplicate name %s for different wavefiles"%wavname

        wav = wave.open(wavfile)
        rate = wav.getframerate()
        rawdata = wav.readframes(wav.getnframes())

        if wav.getsampwidth()==1 : # byte values (unsigned)
            format = 'B'
            max=255
            min=0
        else :
            format='h'
            max=32767
            min=-32768

        d = array.array(format,rawdata)
        # take only first channel. convert sample rate ? or force data ? loop points ?
        data = [255*(d[k]-min)//(max-min)-127 for k in range(0,len(d),wav.getnchannels()) ] # force to i8 ! 

        self.wavs[wavname]=wavfile, rate, data



    def freeze(self) :
        self.frozen=True

    def head(self) :
        self.freeze()
        for n,(fn,fs,data) in self.wavs.items():
            print "const Sample sample_%s; // %2s kHz, %3d kB, %s "%(n,fs/1000,len(data)//1024,os.path.basename(fn))

    def stats(self) :
        self.freeze()
        red4=red0=sumsize=0
        s=[]
        for n,(fn,rate,w) in self.wavs.items() :
            sumsize += len(w)

            # delta-encode wave
            dw = [w[i]-w[i-1] for i in range(1,len(w))]

            # keep 16 most used sampling values 
            mostfreq = Counter(dw).most_common(16) # data, nb
            nb_reduced = sum(n for c,n in mostfreq) 

            # encode them as simple huffman 
            # - bitmap of char data 
            red4 += len(dw)/8
            # - list of raw data
            red4 += len(dw)-nb_reduced
            # - packs of 4-bit index to most encoded table
            red4 += nb_reduced/2
            # - table of 16 values
            red4 += 16

            # ultra simple : encode as 8 bits empty/non empty, data, ...
            red0 += len(dw)/8 # map
            red0 += len(dw)-mostfreq[0][1] # non null


        s.append(("can be reduced4 to (kb)",red4//1024))
        s.append(("can be reduced0 to (kb)",red0//1024))
        s.append(('Total (kb)',sumsize//1024))

        return s

    def export(self) :
        for n,(fn,rate,w) in self.wavs.items() :
            print 'const int8_t sample_%s_data [%d] = {'%(n,len(w))
            for i in range(0,len(w),80) :
                print '        '+','.join('%d'%x for x in w[i:i+80])+','
            print '    };'
            print
            print 'const int sample_%s_len= %d;'%(n,len(w))
            print 'const int sample_%s_rate = %d;'%(n,rate)
            print
            print


s = SoundMgr()
print '#include "stdint.h"'
for snd in glob('sounds/*.wav') : 
    s.add(snd)

s.stats()
s.export()