[spl, freq] = iso226(70);

spl = spl - 70;
peakscale = 1./(10.^(spl./20));

binfreq = 1*44100/512:1*44100/512:145*44100/512;

binpeakscale = spline(freq, peakscale, binfreq);

csvwrite("freqadj_audacious.h", binpeakscale);

exit;
