# LWES Journaller

## Summary
The journaller listens on the given multicast ip and port and writes all
events caught to a file which can be processed later.

## Build Instructions

### Install Requirements
   * popt -- for command line argument processing
   * sys/msg or mqueue messaging
   * lwes core

### Build
```
> % ./bootstrap
> % ./configure --disable-hardcore
> % make
```

### Install
```
> % sudo make install
```

## Usage
```
> lwes-journaller --help
```