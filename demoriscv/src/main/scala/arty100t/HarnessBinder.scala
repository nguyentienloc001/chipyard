package demoriscv.fpga.arty100t

import chisel3._
import freechips.rocketchip.jtag.JTAGIO
import freechips.rocketchip.subsystem.PeripheryBusKey
import freechips.rocketchip.tilelink.TLBundle
import freechips.rocketchip.diplomacy.LazyRawModuleImp
import org.chipsalliance.diplomacy.nodes.HeterogeneousBag
import sifive.blocks.devices.uart.{UARTParams, UARTPortIO}
import sifive.blocks.devices.jtag.{JTAGPins, JTAGPinsFromPort}
import sifive.blocks.devices.pinctrl.BasePin
import sifive.fpgashells.shell._
import sifive.fpgashells.ip.xilinx._
import sifive.fpgashells.shell.xilinx._
import sifive.fpgashells.clocks._
import chipyard.{harness, _}
import chipyard.harness._
import chipyard.iobinders._
import testchipip.serdes._

/*** UART ***/
class WithArty100TUARTHarnessBinder extends HarnessBinder({
  case (th: Arty100THarnessImp, port: UARTPort, chipId: Int) => {
    th.arty100tOuter.io_uart_bb.bundle <> port.io
  }
})

/*** SPI ***/
class WithArty100TSPISDCardHarnessBinder extends HarnessBinder({
  case (th: Arty100THarnessImp, port: SPIPort, chipId: Int) => {
    th.arty100tOuter.io_spi_bb.bundle <> port.io
  }
})

/*** Experimental DDR ***/
class WithArty100TDDRMemHarnessBinder extends HarnessBinder({
  case (th: Arty100THarnessImp, port: TLMemPort, chipId: Int) => {
    val bundles = th.arty100tOuter.ddrClient.out.map(_._1)
    val ddrClientBundle = Wire(new HeterogeneousBag(bundles.map(_.cloneType)))
    bundles.zip(ddrClientBundle).foreach { case (bundle, io) => bundle <> io }
    ddrClientBundle <> port.io
  }
})