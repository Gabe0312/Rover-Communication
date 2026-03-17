import evdev

controller = evdev.InputDevice('/dev/input/event8')
print(f"Successfully connected to: {controller.name}")

for event in controller.read_loop():

    if event.type == 3:
        
        if event.code == 4:
            right_speed = (event.value + 32768) // 256
            print(f"Raw X: {event.value} -> Arduino Byte: {right_speed}")
        
        if event.code == 1:
            left_speed = (event.value + 32768) // 256
            print(f"Raw Y: {event.value} -> Arduino Byte: {left_speed}")
    speed = {
        rightWheel: right_speed,
        leftWheel: left_speed
    }
    json_string = json.dumps(speed)
        