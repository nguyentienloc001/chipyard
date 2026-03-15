package chipyard.fpga.vc707

import chisel3._
import chisel3.experimental.BaseModule
import org.chipsalliance.diplomacy.nodes.HeterogeneousBag
import freechips.rocketchip.tilelink.TLBundle
import sifive.blocks.devices.uart.UARTPortIO
import sifive.blocks.devices.spi.{HasPeripherySPI, SPIPortIO}
import sifive.fpgashells.devices.xilinx.xilinxvc707pciex1.{HasSystemXilinxVC707PCIeX1ModuleImp, XilinxVC707PCIeX1IO}
import chipyard.CanHaveMasterTLMemPort
import chipyard.harness.HarnessBinder
import chipyard.iobinders._
import sifive.fpgashells.shell.ShellJTAGIO

/*** UART ***/
class WithVC707UARTHarnessBinder extends HarnessBinder({
  case (th: VC707FPGATestHarnessImp, port: UARTPort, chipId: Int) => {
    if(port.uartNo == 0) {
      th.vc707Outer.io_uart_bb.bundle <> port.io
    }
    else {
      port.io <> DontCare
    }
  }
})

/*** SPI ***/
class WithVC707SPISDCardHarnessBinder extends HarnessBinder({
  case (th: VC707FPGATestHarnessImp, port: SPIPort, chipId: Int) => {
    if(port.io.toNamed.name == "spi_0") {
      th.vc707Outer.io_spi_bb.bundle <> port.io
    }
    else {
      port.io <> DontCare
    }
  }
})

/*** Experimental DDR ***/
class WithVC707DDRMemHarnessBinder extends HarnessBinder({
  case (th: VC707FPGATestHarnessImp, port: TLMemPort, chipId: Int) => {
    val bundles = th.vc707Outer.ddrClient.out.map(_._1)
    val ddrClientBundle = Wire(new HeterogeneousBag(bundles.map(_.cloneType)))
    bundles.zip(ddrClientBundle).foreach { case (bundle, io) => bundle <> io }
    ddrClientBundle <> port.io
  }
})

/*** JTAG ***/
class WithVC707JTAGHarnessBinder extends HarnessBinder({
  case (th: VC707FPGATestHarnessImp, port: JTAGPort, chipId: Int) => {
    val shellJTAGIO = th.vc707Outer.jtagModule

    port.io.TCK := shellJTAGIO.TCK
    port.io.TMS := shellJTAGIO.TMS
    port.io.TDI := shellJTAGIO.TDI
    shellJTAGIO.TDO.data := port.io.TDO
    shellJTAGIO.TDO.driven := true.B
    shellJTAGIO.srst_n := DontCare
  }
})
