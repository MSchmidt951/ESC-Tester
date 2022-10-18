# ESC-Tester

A program to test up to four ESCs at once with an arduino nano.

## Controls

The board is controlled by my [Multi Purpose Controller](https://github.com/MSchmidt951/Multi-Purpose-Controller).

| Control          | Action                                    |
| -------          | ------                                    |
| Abort            | Turns off all ECSs                        |
| Standby          | Toggle joystick mode                      |
| Light            | Toggle reverse mode                       |
| Left joystick    | Control ESC 1 and 2 when in joystick mode |
| Right joystick   | Control ESC 3 and 4 when in joystick mode |
| Potentiometer    | Control ESCs when not in joystick mode    |
| Buttons          | Toggle induvidual ESCs                    |
| Top right switch | Initialise ECSs                           |

Joystick mode
- Control the ESC percentage with the joysticks when enabled and the potentiometer when disabled

Reverse mode
- This changes where the neutral point (the signal at which the motor is stationary) of the ESC is
- For forward only ESCs disable reverse mode
- For ESCs that can run in both directions enable reverse mode


## LED colours

The small non-RGB LED is active when reverse mode is on.

Each RGB LED corresponds and lines up with an ESC.  
The table below shows what each colour of the RGB LED means.

| LED colour | Meaning         |
| ---------- | -------         |
| Red        | Not sending     |
| Purple     | Starting ESCs   |
| Orange     | Minimum signal  |
| Blue       | Neutral signal  |
| Green      | Maximum signal  |
| Grey/White | Non zero signal |
