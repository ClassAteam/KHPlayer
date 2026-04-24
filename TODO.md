# Most recent commit achievements
- MediaCodec is properly initialized and run on test target Android platform, the poorest hardware config possible ?(Android ARM32).
- The most computationaly heavy codec is benched allright in [bench](./android/playback-evaluation/main.cpp).

# Next step
In next iteration we should develop more heavy testing for booletproof verification that everythings is safe and sound. And delete all the logging related for previous MediaCodec commit after that. 
