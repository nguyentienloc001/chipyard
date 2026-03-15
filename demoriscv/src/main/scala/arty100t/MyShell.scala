package demoriscv.fpga.arty100t

import chisel3._
import chisel3.experimental.dataview.RecordUpcastable
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.prci.{ClockGroup, ClockSinkNode, ClockSinkParameters, ClockSourceNode}
import org.chipsalliance.cde.config.{Field, Parameters}
import sifive.fpgashells.devices.xilinx.xilinxarty100tmig.{XilinxArty100TMIG, XilinxArty100TMIGPads, XilinxArty100TMIGParams}
import sifive.fpgashells.shell._
import sifive.fpgashells.shell.xilinx.{JTAGDebugXilinxPlacedOverlay, SDIOXilinxPlacedOverlay, Series7Shell, SingleEndedClockInputXilinxPlacedOverlay, UARTXilinxPlacedOverlay}


// clock
class SysClockArtyPlacedOverlay(val shell: Arty100TMyShell, name: String, val designInput: ClockInputDesignInput, val shellInput: ClockInputShellInput)
  extends SingleEndedClockInputXilinxPlacedOverlay(name, designInput, shellInput)
{
  val node = shell { ClockSourceNode(freqMHz = 100, jitterPS = 50) }

  shell { InModuleBody {
    val clk: Clock = io
    shell.xdc.addPackagePin(clk, "E3")
    shell.xdc.addIOStandard(clk, "LVCMOS33")
  } }
}
class SysClockArtyShellPlacer(val shell: Arty100TMyShell, val shellInput:ClockInputShellInput)(implicit val valName: ValName)
  extends ClockInputShellPlacer[Arty100TMyShell] {
  def place(designInput: ClockInputDesignInput) = new SysClockArtyPlacedOverlay(shell, valName.name, designInput, shellInput)
}


// uart
class UARTArtyPlacedOverlay(val shell: Arty100TMyShell, name: String, val designInput: UARTDesignInput, val shellInput: UARTShellInput)
  extends UARTXilinxPlacedOverlay(name, designInput, shellInput, false)
{
  shell { InModuleBody {
    val packagePinsWithPackageIOs = Seq(("A9", IOPin(io.rxd)),
      ("D10", IOPin(io.txd)))

    packagePinsWithPackageIOs foreach { case (pin, io) => {
      shell.xdc.addPackagePin(io, pin)
      shell.xdc.addIOStandard(io, "LVCMOS33")
      shell.xdc.addIOB(io)
    } }
  } }
}
class UARTArtyShellPlacer(val shell: Arty100TMyShell, val shellInput: UARTShellInput)(implicit val valName: ValName)
  extends UARTShellPlacer[Arty100TMyShell] {
  def place(designInput: UARTDesignInput) = new UARTArtyPlacedOverlay(shell, valName.name, designInput, shellInput)
}


// spi
//PMOD JA used for SDIO
class SDIOArtyPlacedOverlay(val shell: Arty100TMyShell, name: String, val designInput: SPIDesignInput, val shellInput: SPIShellInput)
  extends SDIOXilinxPlacedOverlay(name, designInput, shellInput)
{
  shell { InModuleBody {
    val packagePinsWithPackageIOs = Seq(("D12", IOPin(io.spi_clk)),
      ("B11", IOPin(io.spi_cs)),
      ("A11", IOPin(io.spi_dat(0))),
      ("D13", IOPin(io.spi_dat(1))),
      ("B18", IOPin(io.spi_dat(2))),
      ("G13", IOPin(io.spi_dat(3))))

    packagePinsWithPackageIOs foreach { case (pin, io) => {
      shell.xdc.addPackagePin(io, pin)
      shell.xdc.addIOStandard(io, "LVCMOS33")
      shell.xdc.addIOB(io)
    } }
    packagePinsWithPackageIOs drop 1 foreach { case (pin, io) => {
      shell.xdc.addPullup(io)
    } }
  } }
}
class SDIOArtyShellPlacer(val shell: Arty100TMyShell, val shellInput: SPIShellInput)(implicit val valName: ValName)
  extends SPIShellPlacer[Arty100TMyShell] {
  def place(designInput: SPIDesignInput) = new SDIOArtyPlacedOverlay(shell, valName.name, designInput, shellInput)
}


// ddr
case object ArtyDDRSize extends Field[BigInt](0x10000000L * 1) // 256 MB
class DDRArtyPlacedOverlay(val shell: Arty100TMyShell, name: String, val designInput: DDRDesignInput, val shellInput: DDRShellInput)
  extends DDRPlacedOverlay[XilinxArty100TMIGPads](name, designInput, shellInput)
{
  val size = p(ArtyDDRSize)

  val ddrClk1 = shell { ClockSinkNode(freqMHz = 166.666)}
  val ddrClk2 = shell { ClockSinkNode(freqMHz = 200)}
  val ddrGroup = shell { ClockGroup() }
  ddrClk1 := di.wrangler := ddrGroup := di.corePLL
  ddrClk2 := di.wrangler := ddrGroup

  val migParams = XilinxArty100TMIGParams(address = AddressSet.misaligned(di.baseAddress, size))
  val mig = LazyModule(new XilinxArty100TMIG(migParams))
  val ddrUI     = shell { ClockSourceNode(freqMHz = 100) }
  val areset    = shell { ClockSinkNode(Seq(ClockSinkParameters())) }
  areset := di.wrangler := ddrUI

  def overlayOutput = DDROverlayOutput(ddr = mig.node)
  def ioFactory = new XilinxArty100TMIGPads(size)

  shell { InModuleBody {
    require (shell.sys_clock.get.isDefined, "Use of DDRArtyPlacedOverlay depends on SysClockArtyPlacedOverlay")
    val (sys, _) = shell.sys_clock.get.get.overlayOutput.node.out(0)
    val (ui, _) = ddrUI.out(0)
    val (dclk1, _) = ddrClk1.in(0)
    val (dclk2, _) = ddrClk2.in(0)
    val (ar, _) = areset.in(0)
    val port = mig.module.io.port

    io <> port.viewAsSupertype(new XilinxArty100TMIGPads(mig.depth))
    ui.clock := port.ui_clk
    ui.reset := !port.mmcm_locked || port.ui_clk_sync_rst
    port.sys_clk_i := dclk1.clock.asUInt
    port.clk_ref_i := dclk2.clock.asUInt
    port.sys_rst := shell.pllReset
    port.aresetn := !(ar.reset.asBool)
  } }

  shell.sdc.addGroup(clocks = Seq("clk_pll_i"), pins = Seq(mig.island.module.blackbox.io.ui_clk))
}
class DDRArtyShellPlacer(val shell: Arty100TMyShell, val shellInput: DDRShellInput)(implicit val valName: ValName)
  extends DDRShellPlacer[Arty100TMyShell] {
  def place(designInput: DDRDesignInput) = new DDRArtyPlacedOverlay(shell, valName.name, designInput, shellInput)
}


// my shell
abstract class Arty100TMyShell()(implicit p: Parameters) extends Series7Shell {
  // PLL reset causes
  val pllReset = InModuleBody { Wire(Bool()) }
  // System clock
  val sys_clock = Overlay(ClockInputOverlayKey, new SysClockArtyShellPlacer(this, ClockInputShellInput()))

  // Peripheries
  val uart = Overlay(UARTOverlayKey, new UARTArtyShellPlacer(this, UARTShellInput()))
  val sdio = Overlay(SPIOverlayKey, new SDIOArtyShellPlacer(this, SPIShellInput()))
  val ddr = Overlay(DDROverlayKey, new DDRArtyShellPlacer(this, DDRShellInput()))
}