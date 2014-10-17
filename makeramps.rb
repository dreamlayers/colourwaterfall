# Program for building tables for computing colours from FFT bins.
# Copyright 2013 Boris Gjenero. Released under the MIT license.

def make_hz last, add, div
    hz = []
    for i in 0..last
        hz[i] = (i+add).to_f*44100/div
    end
    return hz
end

lastbin = ARGV[0].to_i
hz = make_hz lastbin, ARGV[1].to_i, ARGV[2].to_i
pitch = hz.map{ |x|
if x <= 0 then
  0
else
  69+12*Math.log2(x/440)
end
}
midpoint = (pitch[0] + pitch[lastbin])/2

green = pitch.map{ |x| 1-(midpoint-x).abs/(midpoint-pitch[0]) }

puts "#define RGBM_PIVOTBIN " << green.each_with_index.max[1].to_s
puts "#define RGBM_USEBINS " << (lastbin+1).to_s
puts "static const double green_tab[RGBM_USEBINS] = {"
puts green.join(",\n")
puts "};"
