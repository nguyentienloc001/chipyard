package demoriscv.fpga.vc707

import chipyard.harness.HarnessBinder
import chipyard.iobinders._
import chisel3._
import org.chipsalliance.diplomacy.nodes.HeterogeneousBag

/*** UART ***/
class WithVC707UARTHarnessBinder extends HarnessBinder({
  case (th: VC707HarnessImp, port: UARTPort, chipId: Int) => {
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
  case (th: VC707HarnessImp, port: SPIPort, chipId: Int) => {
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
  case (th: VC707HarnessImp, port: TLMemPort, chipId: Int) => {
    val bundles = th.vc707Outer.ddrClient.out.map(_._1)
    val ddrClientBundle = Wire(new HeterogeneousBag(bundles.map(_.cloneType)))
    bundles.zip(ddrClientBundle).foreach { case (bundle, io) => bundle <> io }
    ddrClientBundle <> port.io
  }
})

/*** JTAG ***/
class WithVC707JTAGHarnessBinder extends HarnessBinder({
  case (th: VC707HarnessImp, port: JTAGPort, chipId: Int) => {
    val shellJTAGIO = th.vc707Outer.jtagModule

    port.io.TCK := shellJTAGIO.TCK
    port.io.TMS := shellJTAGIO.TMS
    port.io.TDI := shellJTAGIO.TDI
    shellJTAGIO.TDO.data := port.io.TDO
    shellJTAGIO.TDO.driven := true.B
  }
})
