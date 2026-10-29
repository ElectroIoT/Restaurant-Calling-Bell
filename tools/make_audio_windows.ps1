# Generate PLACEHOLDER announcement WAV files with the Windows speech engine (System.Speech).
# Output: tools\audio-out\MP3\0001.wav ... 0014.wav  (robotic voice; replace with a real recording for production).
# DFPlayer Mini decodes WAV, but its documentation names the folder files 0001.mp3; if your module does not
# pick up 0001.wav, convert with ffmpeg:   ffmpeg -i 0001.wav -codec:a libmp3lame -q:a 4 0001.mp3
# NOTE: this only writes files on your PC; copy them to the microSD card yourself.
Add-Type -AssemblyName System.Speech
$out = Join-Path $PSScriptRoot "audio-out\MP3"
New-Item -ItemType Directory -Force $out | Out-Null
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
$s.Rate = -1
$s.Volume = 100
1..10 | ForEach-Object {
    $s.SetOutputToWaveFile((Join-Path $out ("{0:D4}.wav" -f $_)))
    $s.Speak(("Table {0} is calling for service." -f $_))
}
$extra = @{ 11 = "Attention."; 12 = "Call accepted."; 13 = "Call served."; 14 = "Calling bell ready." }
foreach ($k in $extra.Keys) {
    $s.SetOutputToWaveFile((Join-Path $out ("{0:D4}.wav" -f $k)))
    $s.Speak($extra[$k])
}
$s.Dispose()
Write-Host "Wrote placeholder files to $out"
