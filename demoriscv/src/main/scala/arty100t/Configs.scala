package demoriscv.fpga.arty100t

import freechips.rocketchip.diplomacy._
import freechips.rocketchip.resources.DTSTimebase
import org.chipsalliance.cde.config._
import sifive.blocks.devices.uart.{PeripheryUARTKey, UARTParams}
import sifive.fpgashells.shell.DesignKey
import testchipip.serdes.{SerialTLKey, WithNoSerialTL}
import freechips.rocketchip.devices.tilelink.BootROMLocated
import freechips.rocketchip.subsystem._
import sifive.blocks.devices.spi.{PeripherySPIKey, SPIParams}

import scala.sys.process._

class WithDefaultPeripherals extends Config((site, here, up) => {
  case PeripheryUARTKey => List(UARTParams(address = BigInt(0x64000000L)))
  case PeripherySPIKey => List(SPIParams(rAddress = BigInt(0x64001000L)))
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
  case ExtMem => up(ExtMem, site).map(x => x.copy(master = x.master.copy(size = site(ArtyDDRSize)))) // set extmem to DDR size (note the size)
  case SerialTLKey => Nil // remove serialized tl port
})


// don't use FPGAShell's DesignKey
class WithNoDesignKey extends Config((site, here, up) => {
  case DesignKey => (p: Parameters) => new SimpleLazyModule()(p)
})


class WithArty100TTweaks extends Config(
  // Clock configs
  new chipyard.harness.WithAllClocksFromHarnessClockInstantiator ++
    new chipyard.harness.WithHarnessBinderClockFreqMHz(50) ++
    new chipyard.config.WithMemoryBusFrequency(50.0) ++
    new chipyard.config.WithSystemBusFrequency(50.0) ++
    new chipyard.config.WithPeripheryBusFrequency(50.0) ++
    new chipyard.clocking.WithPassthroughClockGenerator ++
    // Harness Binder
    new WithArty100TUARTHarnessBinder ++
    new WithArty100TSPISDCardHarnessBinder ++
    new WithArty100TDDRMemHarnessBinder ++
    // Peripheris
    new WithDefaultPeripherals ++
    // Other configurations
    new WithNoDesignKey ++
    new WithNoSerialTL ++
    new WithSystemModifications ++
    new chipyard.config.WithNoDebug ++
    new chipyard.config.WithTLBackingMemory ++ // FPGA-shells converts the AXI to TL for us
    new freechips.rocketchip.subsystem.WithExtMemSize(BigInt(256) << 20) ++ // 256mb on ARTY
    new freechips.rocketchip.subsystem.WithoutTLMonitors)


class MTArty100TConfig extends Config(
  new WithArty100TTweaks ++
    new chipyard.config.WithBroadcastManager ++// no l2
    new testchipip.soc.WithNoScratchpads ++
    new freechips.rocketchip.rocket.WithNSmallCores(4) ++
    new chipyard.config.AbstractConfig
)