package demoriscv.fpga.vc707

import freechips.rocketchip.devices.tilelink.BootROMLocated
import freechips.rocketchip.resources.DTSTimebase
import freechips.rocketchip.subsystem.{ExtMem, SystemBusKey}
import org.chipsalliance.cde.config.Config
import sifive.blocks.devices.gpio.{GPIOParams, PeripheryGPIOKey}
import sifive.blocks.devices.spi.{PeripherySPIKey, SPIParams}
import sifive.blocks.devices.uart.{PeripheryUARTKey, UARTParams}
import sifive.fpgashells.shell.xilinx.{VC7074GDDRSize, VC7071GDDRSize}
import testchipip.serdes.SerialTLKey

import scala.sys.process._

class WithDefaultPeripherals extends Config((site, here, up) => {
  case PeripheryUARTKey => List(UARTParams(address = BigInt(0x64000000L)))
  case PeripherySPIKey => List(SPIParams(rAddress = BigInt(0x64001000L)))
  case PeripheryGPIOKey => List(GPIOParams(address = BigInt(0x64002000L), width = 8))
})

class WithSystemModifications extends Config((site, here, up) => {
  case DTSTimebase => BigInt{(1e6).toLong}
  case BootROMLocated(x) => up(BootROMLocated(x), site).map { p =>
    // invoke makefile for sdboot
    val freqMHz = (site(SystemBusKey).dtsFrequency.get / (1000 * 1000)).toLong
    // clean
    val clean = s"make -C demoriscv/src/main/resources/bootROM/MTBoot clean"
    require (clean.! == 0, "Failed to clean")
    // build the BootROM
    val make = s"make -C demoriscv/src/main/resources/bootROM/MTBoot PBUS_CLK=${freqMHz} bin"
    require (make.! == 0, "Failed to build bootrom")
    p.copy(hang = 0x10000, contentFileName = s"./demoriscv/src/main/resources/bootROM/MTBoot/build/sdboot.bin")
  }
  case ExtMem => up(ExtMem, site).map(x => x.copy(master = x.master.copy(size = site(VC7071GDDRSize)))) // set extmem to DDR size (note the size)
  case SerialTLKey => Nil // remove serialized tl port
})

class WithVC707Tweaks extends Config (
  // clocking
  new chipyard.harness.WithAllClocksFromHarnessClockInstantiator ++
    new chipyard.clocking.WithPassthroughClockGenerator ++
    new chipyard.config.WithUniformBusFrequencies(50.0) ++

    new chipyard.harness.WithHarnessBinderClockFreqMHz(50) ++
    new WithFPGAFrequency(50) ++ // default 50MHz freq
    // harness binders
    new chipyard.harness.WithAllClocksFromHarnessClockInstantiator ++
    //  new WithVC707JTAGHarnessBinder ++
    new WithVC707JTAGHarnessBinder ++
    new WithVC707UARTHarnessBinder ++
    new WithVC707SPISDCardHarnessBinder ++
    new WithVC707DDRMemHarnessBinder ++
    // other configuration
    new chipyard.config.WithBroadcastManager ++
    new chipyard.harness.WithI2CTiedOff ++
    new chipyard.iobinders.WithGPIOPunchthrough ++    /** No tie-off GPIOs */
    new WithDefaultPeripherals ++
    new chipyard.config.WithTLBackingMemory ++ // use TL backing memory
    new WithSystemModifications ++ // setup busses, use sdboot bootrom, setup ext. mem. size
//    new chipyard.config.WithNoDebug ++ // remove debug module
    new freechips.rocketchip.subsystem.WithoutTLMonitors ++
    new freechips.rocketchip.subsystem.WithNMemoryChannels(1)
)


// DOC include start: QuadCoreNoCVC707Config
class QuadCoreNoCVC707Config extends Config (
  new WithVC707Tweaks ++
  new chipyard.QuadCoreRing
)
// DOC include end: QuadCoreNoCVC707Config

// DOC include start: QuadCoreXBarVC707Config
class QuadCoreXBarVC707Config extends Config (
  new WithVC707Tweaks++
  new chipyard.ThesisSoC
)
// DOC include end: QuadCoreXBarVC707Config

class TestVC707Config extends Config (
  new WithVC707Tweaks ++
//  new chipyard.cipher.WithPOLY1305(address = 0x10008000) ++
  new freechips.rocketchip.rocket.WithNBigCores(1) ++
  new chipyard.config.AbstractConfig
)

class InternshipConfig extends Config (
  new WithVC707Tweaks ++
  new chipyard.cipher.WithMyTimer(address = 0x00007000) ++
  new chipyard.cipher.WithKLEIN(address = 0x00006000) ++
  new chipyard.config.WithBroadcastManager ++
  new testchipip.soc.WithNoScratchpads ++
  new freechips.rocketchip.rocket.WithNSmallCores(1) ++
  new chipyard.config.AbstractConfig
)

class HeteroCoreConfig extends Config (
  new WithVC707Tweaks ++
  new testchipip.soc.WithNoScratchpads ++
  new freechips.rocketchip.rocket.WithNBigCores(3) ++
  new boom.v3.common.WithNSmallBooms(1) ++
  new chipyard.config.AbstractConfig
)

class WithFPGAFrequency(fMHz: Double) extends Config (
  new chipyard.config.WithPeripheryBusFrequency(fMHz) ++
    new chipyard.config.WithMemoryBusFrequency(fMHz) ++
    new chipyard.config.WithSystemBusFrequency(fMHz) ++
    new chipyard.config.WithControlBusFrequency(fMHz) ++
    new chipyard.config.WithFrontBusFrequency(fMHz)
)

class WithFPGAFreq25MHz extends WithFPGAFrequency(25)
class WithFPGAFreq50MHz extends WithFPGAFrequency(50)
class WithFPGAFreq75MHz extends WithFPGAFrequency(75)
class WithFPGAFreq100MHz extends WithFPGAFrequency(100)
