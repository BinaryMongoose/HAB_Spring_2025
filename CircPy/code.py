# Would work in theory, but untested.
# Why untested? Well, for some reason (probably due to over-abstraction), (or interpreter speeds... )
# the radio is not as efficient as possible. 
# It's a stretch to get ~100m even with antennas.
# So we moved to Arduino. ;(

# This code will forever sit here, unused, until the finality of the heat death of the universe... 
import board
import busio
import microcontroller
import time
import adafruit_rfm9x
import adafruit_ds3231
import adafruit_lis331


# Setting up board connection definitions. All sensors use I2C, radio uses SPI. 
i2c = board.STEMMA_I2C()
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)


# Most efficient way to do this would be to actually sleep, instead of just pausing. 
SLEEP_TIME = 1    

# ALWAYS OPEN IN APPEND MODE. Unless you have a valid reason to write, or you're just reading... 
# (Learned that the hard way... ＞﹏＜ )
with open('/data/LOG.TXT', 'a') as log_file, open('/data/DATA.CSV', 'a') as data_file:
    # Logging if sensors are operational.
    # To be honest, this would be the best time to APPEND sensors to a measurements array if they are operational, but whatever. ¯\_(ツ)_/¯
    try:
        lis = adafruit_lis331.H3LIS331(i2c)
    except Exception as e:
        lis_status = e
    else:
        lis_status = 'OPERATIONAL'
    
    try:
        ds = adafruit_ds3231.DS3231(i2c)
    except Exception as e:
        log_time = 'DS3231 NOT FOUND'
        ds_status = e
    else:
        current = ds.datetime
        log_time = f'{current.tm_mon}/{current.tm_mday}/{current.tm_year} {current.tm_hour}:{current.tm_min}:{current.tm_sec}'
        ds_status = 'OPERATIONAL'
    
    
    # MASS WRITE. Wonder how long it takes... 
    log_file.write(f'{time.monotonic()}: Session started, date is {log_time}\n')
    log_file.write(f'{time.monotonic()}: LIS331 status {lis_status}\n')
    log_file.write(f'{time.monotonic()}: DS3231 status {ds_status}\n')
    log_file.write(f'{time.monotonic()}: LoRa status {}\n')
    log_file.write(f'{time.monotonic()}: Header created\n')
    log_file.flush()
    
    # Improvement would be a dynamic header that matches the measurement array. 
    data_file.write('time.monotonic, time, accel_x, accel_y, accel_z, micro_temp\n')
    data_file.flush()
    
    # Transmit initial data packet
    # Or... not. (Again, radio issues.)
    

# Biggest brain moment, store lambdas in an array. You could just use actual functions, but these are just one-liners. 
measurements = [lambda: f'{ds.datetime[3]}:{ds.datetime[4]}:{ds.datetime[5]}', lambda: lis.acceleration[0], lambda: lis.acceleration[1], lambda: lis.acceleration[2], lambda: microcontroller.cpu.temperature]


# Why constantly reopen the file? Sure, it would make the code faster, but in my experience, 
# the only way you can ensure file reliability is to close & reopen files. And if you're concerned about FLASH wear, 
# this code is only intended to run for about ~4 hours. Then the microcontroller will sit on a shelf somewhere, collecting dust. 

# Though I would be interested to hear if doing this could lead to corruption. But corruption is like cancer—computers get corrupted just by existing. 

# With the state of things now, it takes 0.18120237 + SLEEP_TIME seconds for the loop to complete. On an RP2040. 
while True:
    with open('/data/LOG.TXT', 'a') as log_file, open('/data/DATA.CSV', 'a') as data_file:
        now = time.monotonic() # Even though the writes are technically not at the same time, it makes parsing errors later a whole lot easier, which is what the timestamps are for, right? 
        data_file.write(f'{now}, ') # Starting the CSV line. 
        for m in measurements:
            # If good, just print the value. 
            try:
                data_file.write(f'{m()}, ')
                print(f'{m()}, ', end='') # Printing for serial sniffers. 
            except OSError as e: # OSError is when the sensor WAS initialized, but is now missing. 
                log_file.write(f'{now}: Device disconnected! {e}\n')  
            except AttributeError as e: # AttributeError is for when the sensor was never set up in the first place. 
                log_file.write(f'{now}: Device wasn\'t initialized! {e}\n')
            except Exception as e: # And this is a catch-all in case of a funky mishap. (Shouldn't ever be triggered, just in case.)
                log_file.write(f'{now}: Unknown error occurred! {e}\n')
        
        # Gotta get ready to print a new line.
        # (Though I could just start with a data_file.write(f'\n{now}, '))... 
        # I think this makes it clearer that we're finished with the line, though. 
        print()
        data_file.write('\n')
        # Flushing is the actual proper term. Whoever came up with it is a genius. 
        data_file.flush()
        log_file.flush()
        
    # Again, actual sleeping here would be most efficient. Though we are using a 3500mAh battery, so power conservation isn't a main concern. 
    time.sleep(SLEEP_TIME)
