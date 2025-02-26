import board
import digitalio
import storage

button = digitalio.DigitalInOut(board.BUTTON)
button.switch_to_input(pull=digitalio.Pull.UP)

storage.remount("/", readonly=not button.value)