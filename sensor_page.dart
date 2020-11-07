import 'dart:async';
import 'dart:convert' show utf8;

import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';

class SensorPage extends StatefulWidget {

  const SensorPage({Key key, this.device}): super(key: key);
  final BluetoothDevice device;

  @override
  _SensorPageState createState() => _SensorPageState();
}

class _SensorPageState extends State<SensorPage> {
  final String SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  final String CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  final String TARGET_DEVICE_NAME = "ESP32 Boi";
  bool isReady;
  Stream<List<int>> stream;
  List<double> traceVoltage = List();
  final myController = TextEditingController();

  FlutterBlue flutterBlue = FlutterBlue.instance;
  StreamSubscription<ScanResult> scanSubscription;

  BluetoothDevice targetDevice;
  BluetoothCharacteristic targetCharacteristic;

  String connectionText = "";

  @override
  void dispose() {
    // Clean up the controller when the widget is disposed.
    myController.dispose();
    super.dispose();
  }

  @override
  void initState() {
    super.initState();
    isReady = false;
    connectToDevice();
  }

  connectToDevice() async {
    if (widget.device == null) {
      _Pop();
      return;
    }

    new Timer(const Duration(seconds: 15), () {
      if (!isReady) {
        disconnectFromDevice();
        _Pop();
      }
    });

    await widget.device.connect();
    discoverServices();
  }
  disconnectFromDevice() {
    if (widget.device == null) {
      _Pop();
      return;
    }

    widget.device.disconnect();
  }

  discoverServices() async{
    if(widget.device == null){
      _Pop();
      return;
    }

    List<BluetoothService> services = await widget.device.discoverServices();
    services.forEach((service){
      if(service.uuid.toString()==SERVICE_UUID){
        service.characteristics.forEach((characteristic){
          if(characteristic.uuid.toString() == CHARACTERISTIC_UUID){
            characteristic.setNotifyValue(!characteristic.isNotifying);
            stream = characteristic.value;

            targetCharacteristic = characteristic;
            writeData("Hi there, ESP32!!");

            setState(() {
              isReady = true;
            });
          }
        });
      }
    });
    if(!isReady){
      _Pop();
    }
  }

  writeData(String data) {
    if (targetCharacteristic == null) return;

    List<int> bytes = utf8.encode(data);
    targetCharacteristic.write(bytes);
  }

  Future<bool> _onWillPop(){
    return showDialog(
        context: context,
        builder: (context)=>
        new AlertDialog(
          title: Text('Are you sure?'),
          content: Text('Do you want to disconnect device and go back?'),
          actions: <Widget>[
            new FlatButton(onPressed: ()=> Navigator.of(context).pop(false), child: new Text("No")),
            new FlatButton(onPressed: (){
              disconnectFromDevice();
              Navigator.of(context).pop(true);
            },
                child: new Text('Yes')),
          ],
        )??
            false);
  }

  _Pop(){
    Navigator.of(context).pop(true);
  }

  String _dataParser(List<int> dataFromDevice){
    return utf8.decode(dataFromDevice);
  }

  @override
  Widget build(BuildContext context) {

    return WillPopScope(
      onWillPop: _onWillPop,
      child: Scaffold(
        appBar: AppBar(title: Text('Voltage 1 Measurements'),
        ),
        body: Container(child: !isReady ? Center(child: Text(
          'Waiting...',
          style: TextStyle(fontSize: 24, color: Colors.red),
        ),
        )
            : Container(
          child: StreamBuilder<List<int>>(
            stream: stream,
            builder: (BuildContext context,
                AsyncSnapshot<List<int>> snapshot){
              if(snapshot.hasError)
                return Text('Error: ${snapshot.error}');
              if(snapshot.connectionState == ConnectionState.active){
                var voltageValue = _dataParser(snapshot.data);
                traceVoltage.add(double.tryParse(voltageValue) ?? 0);

                return Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: <Widget>[
                        Expanded(flex:1 ,child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: <Widget>[
                              Text('Live Voltage from Device: ', style: TextStyle(fontSize: 14)),
                              Text('$voltageValue V', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 24))
                            ]
                        ),),
                        Expanded(flex: 1, child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: <Widget>[
                            Text('Enter Triggering Voltage in kV: ', style: TextStyle(fontSize: 14)),
                            TextField(controller: myController,),
                            OutlineButton(
                              textColor: Colors.blue,
                              highlightedBorderColor: Colors.black.withOpacity(0.12),
                              onPressed: () {
                                // Respond to button press
                                var trigger_v= double.tryParse(myController.text);
                                print(trigger_v);
                                writeData("$trigger_v");
                              },
                              child: Text("Send trigger voltage"),
                            )
                          ],
                        )
                        ),
                        Expanded(flex: 1, child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: <Widget>[
                            Text("Trigger", style: TextStyle(fontSize: 14)),
                            OutlineButton(
                              textColor: Colors.blue,
                              highlightedBorderColor: Colors.black.withOpacity(0.12),
                              onPressed: () {
                                // Respond to button press
                                writeData("f");
                              },
                              child: Text("Fire"),
                            )
                          ],
                        ))
                      ],)
                );
              }else{
                return Text('Check the stream');
              }
            },
          ),
        )),
      ),
    );
  }
}
