# MiniEDR by @j3h4ck

![Project Logo](./MiniEDR.PNG)

## Overview

This project is a **fun and educational** experiment to dive deep into **malware development** by understanding how the **Windows kernel** and **Endpoint Detection and Response (EDR)** mechanisms work.  
It monitors **process creation and termination**, sending details to a user-mode application through a **named pipe**.

## Features

- ✅ Monitors **process creation** and **termination**
- ✅ Retrieves **process image name, parent process, command line, and session ID**
- ✅ Sends data to a **user-mode application** via a **named pipe**
- ✅ Built **for learning and research purposes**

## Components

- **Kernel Driver** 🖥️: Hooks into process creation and termination events.
- **User-mode Application** 📡: Reads process activity data from the named pipe.

## Installation & Usage

1. **Load the kernel driver** using a driver loader.
2. **Run the user-mode application** to start monitoring.
3. **Observe process activity** in real-time.

## Disclaimer ⚠️

This project is **for educational and research purposes only**.  
It should **not be used for malicious activities**.

## License 📜

[MIT License](LICENSE)

---

⭐ **Feel free to contribute and explore how EDRs function at a lower level!** ⭐
