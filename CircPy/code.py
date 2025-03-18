# Software that detects wether sensors reading are bad dnamically! You're a certified genius!
import board
import busio
import time
import adafruit_rfm9x
import adafruit_ds3231
import adafruit_lis331
import microcontroller

i2c = board.STEMMA_I2C()
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)


SLEEP_TIME = 1
    
def log_file(time, *args):
    # The only reason we accept time is so we can keep line numbers consistent. 
    return f'{time}: {args[0]} {args}\n'
    
    

with open('/data/LOG.TXT', 'a') as log_file, open('/data/DATA.CSV', 'a') as data_file:
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

    log_file.write(f'{time.monotonic()}: Session started, date is {log_time}\n')
    log_file.write(f'{time.monotonic()}: LIS331 status {lis_status}\n')
    log_file.write(f'{time.monotonic()}: DS3231 status {ds_status}\n')
    log_file.write(f'{time.monotonic()}: LoRa status {}\n')
    log_file.write(f'{time.monotonic()}: Header created\n')
    log_file.flush()
    
    data_file.write('time.monotonic, time, accel_x, accel_y, accel_z, micro_temp\n')
    data_file.flush()
    
    # Transmit inital data packet
    

measurements = [lambda: f'{ds.datetime[3]}:{ds.datetime[4]}:{ds.datetime[5]}', lambda: lis.acceleration[0], lambda: lis.acceleration[1], lambda: lis.acceleration[2], lambda: microcontroller.cpu.temperature]

# With the state of things now it takes 0.18120237 + SLEEP_TIME seconds for the loop to complete. (Without sleep)
while True:
    with open('/data/LOG.TXT', 'a') as log_file, open('/data/DATA.CSV', 'a') as data_file:
        now = time.monotonic()
        data_file.write(f'{now}, ')
        for m in measurements:
            try:
                data_file.write(f'{m()}, ')
                print(f'{m()}, ', end='')
            except OSError as e:
                log_file.write(f'{now}: Device disonnected! {e}\n')
            except AttributeError as e:
                log_file.write(f'{now}: Device wasn\'t initialized! {e}\n')
            except Exception as e:
                log_file.write(f'{now}: Unknown error occured! {e}\n')
                
        print()
        data_file.write('\n')
        data_file.flush()
        log_file.flush()
        
    time.sleep(SLEEP_TIME)
        