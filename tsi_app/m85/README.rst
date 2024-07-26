.. _m85:

TSI Banner
###########

Overview
********

This is the core M85 Zephyr platform and initial startup code. It prints TSI Banner to the console by invocation of Zephyr main. This module also enables the logging functionality  to log messages to console at different levels of severity such as Info, Warning, Error & Debug.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: tsi_app/m85
   :host-os: unix
   :board: ek_tsi_skyp/m85
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console
!! ------------------------------------------ !!
!! WELCOME TO TSAVORITE SCALABLE INTELLIGENCE !!
!! ------------------------------------------ !! 
[00:00:00.010,000] <inf> m85: Test Platform: ek_tsi/skyp/m85
[00:00:00.010,000] <wrn> m85: Testing on FPGA; Multi module init TBD 
