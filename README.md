# SivinPlayer
基于FFMPEG进行解码播放的播放器,本项目主要功能是深入播放器原理,重点是在视频音频的解码,以及视频渲染功能
主要提供两种解码思路,一种是利用FFMPEG进行软解,一种是利用Android 平台本身的硬解码来进行解码

显示渲染,提供两种思路,一种是原生NativeWindows的方式,一种是OpenGL渲染的方式

同时提供Java层实现和C/C++实现