//
//  ViewController.swift
//  WakeMe
//
//  Created by Philipp Junger on 09.11.20.
//

// Import
import CoreBluetooth
import UIKit
import MessageUI
import Charts
import TinyConstraints
import AVFoundation
import MediaPlayer


// ViewController
let ExportController: SecondViewController = SecondViewController()
let StatusController: ViewController = ViewController()
let VerlaufController: ThirdViewController = ThirdViewController()
let AlarmgrenzeController: FourthViewController = FourthViewController()

// CB (CoreBluetooth) Variablen initiieren
let char1CBUUID = CBUUID(string: "FFE1")
var centralManager: CBCentralManager!
var myPeripheral: CBPeripheral!
var characteristicselected: CBCharacteristic?


// Setting
var defaults = UserDefaults.standard
var fileURL = URL(string: "Platzhalter")


// Arrays initialisieren
var strarray = Array(["Platzhalter", "Platzhalter", "Platzhalter", "Platzhalter", "Platzhalter"])
var spo2arr: [String] = []
var bpmarr: [String] = []
var batteryarr: [String] = []
var statusarr: [String] = []
var datearr: [String] = []
var timearr: [String] = []
var timearrjusttime: [String] = []
var csvarr:[String] = []
var alarmgrenzearr:[String] = []

// Farben initialisieren
let mygreen =  UIColor(red: 51/255, green: 161/255, blue: 57/255, alpha: 1)
let myred =  UIColor(red: 210/255, green: 35/255, blue: 35/255, alpha: 1)
let myorange =  UIColor(red: 166/255, green: 86/255, blue: 35/255, alpha: 1)
let myblue = UIColor(red: 46/255, green: 85/255, blue: 148/255, alpha: 1)


// Spalten setzen
var spalte_spo2 = 2
var spalte_time = 1
var spalte_bpm = 3

// Alarmgrenze
var alarmgrenze: [UInt8] = [0]
var alarmgrenze_last: [UInt8] = [0]

// Audio
var AlarmBuzzer: AVAudioPlayer?

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - - - -ViewController - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///

class ViewController: UIViewController, CBCentralManagerDelegate, CBPeripheralDelegate, MFMailComposeViewControllerDelegate
{
    
    // Verknüpfung Label mit UI
    @IBOutlet weak var zeitLabel: UILabel!
    @IBOutlet weak var SpO2Label: UILabel!
    @IBOutlet weak var BPMLabel: UILabel!
    @IBOutlet weak var BatteryLabel: UILabel!
    @IBOutlet weak var StatusLabel: UILabel!
    @IBOutlet weak var ConnectLabel: UILabel!
    @IBOutlet weak var activityIndicator: UIActivityIndicatorView!
    @IBOutlet weak var whitefront: UILabel!
    @IBOutlet weak var Whitefrontback: UILabel!
    
    
    // Suche nach BLE Service
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        
        for service in services {
        print(service)
        peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    
    // Suche nach BLE Characteristics
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
      guard let characteristics = service.characteristics else { return }

      for characteristic in characteristics {
        print(characteristic)
        characteristicselected = characteristic
        print("characteristicselected gesetzt")
        if characteristic.properties.contains(.read) {
          print("\(characteristic.uuid): properties contains .read")
            peripheral.readValue(for: characteristic)
            
        }
        if characteristic.properties.contains(.notify) {
          print("\(characteristic.uuid): properties contains .notify")
            // Subscribe to notification
            peripheral.setNotifyValue(true, for: characteristic)

        }
      }
    }

    // Überwachung des Device-BLE Status
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
            if central.state == CBManagerState.poweredOn {
                print("BLE powered on")
                central.scanForPeripherals(withServices: nil, options: nil)
                // Turned on
            }
            else {
                print("Something wrong with BLE")
                // Not on, but can have different issues
                ConnectLabel.textColor = myred
                ConnectLabel.text = "Bitte Bluetooth einschalten"
            }
        }
    
   
    // Suche nach WakeMe, Scan stoppen und mit WakeMe verbinden
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if let pname = peripheral.name {
            if (pname == "WakeMe") || (pname == "HMSoft") {
                WakeMe.centralManager.stopScan()
            
                myPeripheral = peripheral
                myPeripheral.delegate = self
                WakeMe.centralManager.connect(peripheral, options: nil)
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Mit WakeMe verbunden")
        myPeripheral.discoverServices(nil)
        ConnectLabel.text = "Verbunden"
        ConnectLabel.textColor = mygreen
        activityIndicator.stopAnimating()
        }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Verbindung unterbrochen")
        ConnectLabel.text = "Verbindung unterbrochen \nSuche nach WakeMe..."
        ConnectLabel.textColor = myred
        StatusLabel.text = "- -"
        
        zeitLabel.text = "- - -"
        SpO2Label.text = "- - -"
        BPMLabel.text = "- - -"
        BatteryLabel.text = "- - -"
        
        // ActivityIndicator starten wenn Verbindung unterbrochen
        activityIndicator.startAnimating()
        WakeMe.centralManager.connect(myPeripheral)
    }
    
    func sendData(datasource: [UInt8]) {
        let data = NSData(bytes: datasource, length: datasource.count)
        //print(data)
        if (characteristicselected != nil) {
        myPeripheral.writeValue(data as Data, for: characteristicselected!, type: .withoutResponse)
        }
        else
        {print("Error")}
    }
    
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        WakeMe.centralManager = CBCentralManager(delegate: self, queue: nil)
        self.Whitefrontback.UILableTextShadow()
        self.whitefront.UILabelCorner()
        overrideUserInterfaceStyle = .dark
        
//        // User Notifications
//        // Step 1: Ask for permission
//        let center = UNUserNotificationCenter.current()
//        center.requestAuthorization(options: [.alert, .sound]) { (granted, error) in
//        }
//
//        // Step 2: Create the notification content
//        let content = UNMutableNotificationContent()
//        content.title = "WakeMe Alarm"
//        content.body = "Look at me!"
//        //Default sound
//        //content.sound = UNNotificationSound.default
//        //Play custom sound
//        content.sound = UNNotificationSound.init(named:UNNotificationSoundName(rawValue: "security.caf")) // Muss im Unterordner "WakeMe" liegen
//
//        // Step 3: Create the notification trigger
//        let date = Date().addingTimeInterval(10)
//        let dateComponents = Calendar.current.dateComponents([.year, .month, .day, .hour, .minute, .second], from: date)
//        let trigger = UNCalendarNotificationTrigger(dateMatching: dateComponents, repeats: false)
//
//        // Step 4: Create the request
//        let uuidString = UUID().uuidString
//        let request = UNNotificationRequest(identifier: uuidString, content: content, trigger: trigger)
//
//        // Step 5: Register the request
//        center.add(request) { (error) in
//            // Check the error parameter and handle any errors
//        }
}

    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?)
    {
        switch characteristic.uuid
      {
        
        case char1CBUUID: // Wenn UUID = FFE1, dann ist es unser Gerät, also auslesen
            //print(characteristic.value ?? "no value")
            // Convert Serial Data to String
  
            let str = String(data: characteristic.value!, encoding: .utf8) ?? "nothing"
            let strarray = str.components(separatedBy: " ")
            
            // Wenn ein Wert für Alarmgrenze gesetzt wurde, schicke ihn an das Gerät
            if ((alarmgrenze[0] != 0) && (alarmgrenze_last != alarmgrenze))
            {
                sendData(datasource: alarmgrenze)
                print("sendData(): \(alarmgrenze)")
                
            }
        
            // Um Lesefehler zu ignorieren
            if (strarray.count == 6)
            {
                print(strarray)
                let batterylevel = (Float(strarray[3]) ?? 0)/100
                // Umrechnung in %: Für 3.3V (0%) und 4.2V (100%):
                let batterylevelpercentage = ((batterylevel - 3.3)/(4.2-3.3)) * 100;
                
                // Umwandlung in Text für UI Ausgabe
                zeitLabel.text = ExportController.zeitausgeben(x:"T")
                SpO2Label.text = "\(strarray[1]) %"
                BPMLabel.text = strarray[2]
                BatteryLabel.text = "\(String(batterylevel)) V (\(String(format: "%.0f", batterylevelpercentage))%)"
                
                // Funktionszustand
                switch strarray[4] {
                case String(0):
                    StatusLabel.text = "Gerätefehler, bitte wenden Sie sich umgehend an eine Servicestelle"
                    StatusLabel.textColor = myred
                    zeitLabel.text = "- -"
                    SpO2Label.text = "- -"
                    BPMLabel.text = "- -"
                    BatteryLabel.text = "- -"
                case String(1):
                    StatusLabel.text = "Überwachung aktiv"
                    StatusLabel.textColor = mygreen
                case String(2):
                    StatusLabel.text = "Sensorposition prüfen"
                    StatusLabel.textColor = myorange
                case String(3):
                    StatusLabel.text = "Gerät bereit"
                    StatusLabel.textColor = mygreen
                case String(4):
                    StatusLabel.text = "A L A R M"
                    StatusLabel.textColor = myorange
                    
                    // Set Volume to max
                    MPVolumeView.setVolume(0.1)
                    
                    let path = Bundle.main.path(forResource: "security.caf", ofType:nil)!
                    let url = URL(fileURLWithPath: path)
                    
                    // Enable sound playing also in background
                    do {
                        try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default, options: [.mixWithOthers, .allowAirPlay])
                        print("Playback OK")
                        try AVAudioSession.sharedInstance().setActive(true)
                        print("Session is Active")
                    } catch {
                        print(error)
                    }

                    // Play Alarm sound
                    do {
                        AlarmBuzzer = try AVAudioPlayer(contentsOf: url)
                        AlarmBuzzer?.play()
                    } catch {
                        // couldn't load file :(
                    }
                    
                    // Start Vibration
                    UIDevice.vibrate()

                case String(5):
                    StatusLabel.text = "Alarmpause durch Nutzer für 10 Sekunden"
                    StatusLabel.textColor = myorange
                case String(6):
                    StatusLabel.text = "Batterieladung zu gering, bitte Gerät aufladen"
                    StatusLabel.textColor = myorange
                default:
                    StatusLabel.text = "Funktionsfehler Default"
                    StatusLabel.textColor = myred
                }

                // Erstelle ein String-Array mit allen empfangenen Werten
                spo2arr.append(strarray[1])
                bpmarr.append(strarray[2])
                batteryarr.append(String(batterylevel))
                statusarr.append(strarray[4])
                // Echtzeit im Array mitspeichern speichern
                timearr.append(ExportController.zeitausgeben(x: "T"))
                datearr.append(ExportController.zeitausgeben(x: "D"))
                timearrjusttime.append(ExportController.zeitausgeben(x: "Tshort"))
                alarmgrenzearr.append(strarray[5])
                
                // CSV Datei erstellen
                ExportController.createCSV()
            }
            
            default:
            print("Unhandled Characteristic UUID: \(characteristic.uuid)")
      }
    }
}

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - SECOND VIEW CONTROLLER  - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - -EXPORT          - - - - - - - - - - - - - - - - - - - - - ///

class SecondViewController: UIViewController, MFMailComposeViewControllerDelegate, UIPickerViewDelegate, UIPickerViewDataSource
{
    lazy var pickerAuswahl = pickerData?.last
    
    // Verknüpfung Button mit UI
    @IBAction func ExportButton(_ sender: Any) {
        print("Daten versenden Button pressed")
        print("pickerAuswahl: " + pickerAuswahl!)
        sendEmail(ValueSelected: pickerAuswahl!)
    }
    
    // CSV Datei Picker
    @IBOutlet weak var picker: UIPickerView!

    lazy var pickerData = listofCSVfiles(extensionWanted: "csv")
    
    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }
    
    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return pickerData?.count ?? 0
    }

    func pickerView(_ pickerView: UIPickerView, viewForRow row: Int, forComponent component: Int, reusing view: UIView?) -> UIView {
        var label = UILabel()
            if let v = view {
                label = v as! UILabel
            }
            //label.font = UIFont (name: "Helvetica Neue", size: 16)
            label.text =  pickerData?[row]
            label.textAlignment = .center
            return label
        }
    
     func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent  component: Int) {
        pickerAuswahl = pickerData![row] as String
        print(pickerAuswahl!)
    }
    

    // Zeit und Datum ausgeben, DT = Datum und Zeit, D = nur Datum, T = nur Zeit
    func zeitausgeben(x: String) -> String {
        let now = Date()
        let formatter = DateFormatter()
        formatter.timeZone = TimeZone.current
            if (x == "DT"){
                formatter.dateFormat = "dd.MM.yyyy HH:mm:ss"
            }
            else if (x == "D"){
                formatter.dateFormat = "dd.MM.yyyy"
            }
            else if (x == "D-rev"){
                formatter.dateFormat = "yyyy.MM.dd"
                // Zum beispielhaften Erzeugen von Datensätzen verschiedener Tage
                //formatter.dateFormat = "2020.11.10"
            }
            else if (x == "T"){
                formatter.dateFormat = "HH:mm:ss"
            }
            else if (x == "Tshort"){
                formatter.dateFormat = "HH:mm"
            }
            else {print("Fehler in zeitausgeben()")}
        let dateString = formatter.string(from: now)
        return dateString
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        // datePicker
        self.picker.dataSource = self
        self.picker.delegate = self
        
        let last = (pickerData?.count ?? 2)-1
        //print(last)
        self.picker.selectRow(last, inComponent: 0, animated: true)
        picker.reloadAllComponents()
        overrideUserInterfaceStyle = .dark

    }
    
    override func viewDidAppear(_ animated: Bool) {
        print("viewDidAppear")
        picker.reloadAllComponents()

    }
    
    func createCSV() {
        ////////////////////////////////////////////////////////////////////////////////  CSV DATEI ////////////////////////////////////////////////////////////////////////////////
        // Erstelle ein Gesamtarray für CSV Ausgabe mit aktuellen Werten
        csvarr.removeAll()
        csvarr.append(datearr.last!)
        csvarr.append(timearr.last!)
        csvarr.append(spo2arr.last!)
        csvarr.append(bpmarr.last!)
        csvarr.append(batteryarr.last!)
        csvarr.append(statusarr.last!)
        csvarr.append(alarmgrenzearr.last!)

        
        // Erstelle CSV-Heading
        let heading = "Datum, Zeit, SpO2, BPM, Battery, Status, Alarmgrenze\n"
        var neuezeile = "0,0,0,0,0,0,0,0\n"
             
        do {
            let path = try FileManager.default.url(for: .documentDirectory,
                                                   in: .allDomainsMask,
                                                   appropriateFor: nil,
                                                   create: false)
            fileURL = path.appendingPathComponent("Messwerte-WakeMe-" + zeitausgeben(x:"D-rev") + ".csv")
            
            //Check if file exists
                do {
                    let fileHandle = try FileHandle(forWritingTo: fileURL!)
                        fileHandle.seekToEndOfFile()
                        // Bei existierender Datei nur Daten anhängen
                        neuezeile = csvarr.joined(separator: ",") + "\n"
                        fileHandle.write(neuezeile.data(using: .utf8)!)
                        fileHandle.closeFile()
                        
                    defaults.set(zeitausgeben(x:"D-rev"), forKey: "lastcsvdate")
                    }
                catch {
                
                // If not, create File
                    print("Create new file")
                    try! "".write(to: fileURL!, atomically: true, encoding: String.Encoding.utf8)
                    let fileHandle = try FileHandle(forWritingTo: fileURL!)
                    // Bei neuer Datei Überschriften hinzufügen
                    neuezeile = heading + csvarr.joined(separator: ",") + "\n"
                    fileHandle.write(neuezeile.data(using: .utf8)!)
                    print("Create new file - erste Zeile geschrieben")
                    defaults.set(zeitausgeben(x:"D-rev"), forKey: "lastcsvdate")
                }
        }
        catch {
            print("error creating file")
        }
        ////////////////////////////////////////////////////////////////////////////////  ENDE //////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////  CSV DATEI ////////////////////////////////////////////////////////////////////////////////

    }
    
    ////////////////////////////////////////////////////////////////////////////////  START //////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////  PICKER ////////////////////////////////////////////////////////////////////////////////
    // Liste der CSV Dateien im Ordner ausgeben
    func listofCSVfiles(extensionWanted: String) -> [String]? {

        let Path = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
    
        do {
            try FileManager.default.createDirectory(atPath: Path.relativePath, withIntermediateDirectories: true)
            // Get the directory contents urls (including subfolders urls)
            let directoryContents = try FileManager.default.contentsOfDirectory(at: Path, includingPropertiesForKeys: nil, options: [])

            // if you want to filter the directory contents you can do like this:
            let FilesPath = directoryContents.filter{ $0.pathExtension == extensionWanted }
            let FileNames = FilesPath.map{ $0.deletingPathExtension().lastPathComponent }
            
            //print(FileNames)
            
            let SortedFileNames = FileNames.sorted()
            print(SortedFileNames)
            return SortedFileNames

        } catch {
            print(error.localizedDescription)
        }
        return nil
    }
    ////////////////////////////////////////////////////////////////////////////////  ENDE //////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////  PICKER ////////////////////////////////////////////////////////////////////////////////


    // Versenden der Messwerte als .csv per Mail
    func sendEmail(ValueSelected: String) {
        //print ValueSelected
        
        // locate folder containing CSV file
       let documentsPath = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.documentDirectory, FileManager.SearchPathDomainMask.userDomainMask, true)[0]
       let CSVFileNameToBeAdded = (documentsPath as NSString).appendingPathComponent(ValueSelected + ".csv")
       let fileData = NSData(contentsOfFile: CSVFileNameToBeAdded)
        
        // eMail vorausfüllen
        if MFMailComposeViewController.canSendMail() {
        let mail = MFMailComposeViewController()
        mail.mailComposeDelegate = self
        mail.setToRecipients(["mail@philippjunger.de"])
        mail.setSubject(ValueSelected)
        mail.setMessageBody("<p>Aufgezeichnete Daten vom WakeMe Prototyp</p>", isHTML: true)
        mail.addAttachmentData(fileData! as Data,  mimeType: "text/csv", fileName: (ValueSelected + ".csv"))
            present(mail, animated: true)
        } else {
            print("Fehler beim Versenden der Mail")
        }
    }

    func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith result: MFMailComposeResult, error: Error?) {
        controller.dismiss(animated: true)
    }
}

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - VERLAUF VIEW CONTROLLER  - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///

class ThirdViewController: UIViewController, ChartViewDelegate, UIPickerViewDelegate, UIPickerViewDataSource {
    
    // PickerVerlauf initiieren
    lazy var pickerAuswahl = pickerData?.last
    
    // CSV Datei Picker
    @IBOutlet weak var picker: UIPickerView!
    lazy var pickerData = ExportController.listofCSVfiles(extensionWanted: "csv")
    
    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }
    
    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return pickerData?.count ?? 0
    }

    func pickerView(_ pickerView: UIPickerView, viewForRow row: Int, forComponent component: Int, reusing view: UIView?) -> UIView {
            var label = UILabel()
            if let v = view {
                label = v as! UILabel
            }
            //label.font = UIFont (name: "Helvetica Neue", size: 16)
            label.text =  pickerData?[row]
            label.textAlignment = .center
            return label
        }
    
     func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent  component: Int) {
        pickerAuswahl = pickerData![row] as String
        print(pickerAuswahl!)
        setData()
     }
    
    // Spalten aus CSV Dateien herauslesen
    func csvwerteausgeben(pickerAuswahl: String, Spalte: Int) -> [String] {
        var data = readDataFromCSV(fileName: pickerAuswahl)
        data = cleanRows(file: data!)
        let csvRows = csv(data: data!)
        
        var csvColumns: [String] = []
        for i in 1 ..< (csvRows.count - 1) {
            let neuezeile = csvRows[i][Spalte]
            csvColumns.append(neuezeile)
        }
        return csvColumns
    }
    
    func readDataFromCSV(fileName:String)-> String!{
        // locate folder containing CSV file
           let documentsPath = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.documentDirectory, FileManager.SearchPathDomainMask.userDomainMask, true)[0]
           let CSVFileNameToBeAdded = (documentsPath as NSString).appendingPathComponent(fileName + ".csv")
        
            do {
                let contents = try String(contentsOfFile: CSVFileNameToBeAdded, encoding: .utf8)
                //print("readDataFromCSV ok")
                return contents
            } catch {
                print("File Read Error for file \(CSVFileNameToBeAdded)")
                return nil
            }
        }
    
    func cleanRows(file:String)->String{
        var cleanFile = file
        cleanFile = cleanFile.replacingOccurrences(of: "\r", with: "\n")
        cleanFile = cleanFile.replacingOccurrences(of: "\n\n", with: "\n")
        return cleanFile
    }
    
    func csv(data: String) -> [[String]] {
        var result: [[String]] = []
        let rows = data.components(separatedBy: "\n")
        for row in rows {
            let columns = row.components(separatedBy: ",")
            result.append(columns)
        }
        return result
    }
    
    
    // LineChart initiieren
    lazy var lineChartView: LineChartView = {

        let chartView = LineChartView()
        //chartView.backgroundColor = .systemBlue
        chartView.rightAxis.enabled = false
        chartView.highlightValue(nil, callDelegate: false)
        
        let yAxis = chartView.leftAxis
        yAxis.labelFont = .boldSystemFont(ofSize: 12)
        yAxis.setLabelCount(7, force: false)
        //yAxis.labelTextColor
        //yAxis.axisLineColor = .white
        yAxis.labelPosition = .outsideChart
        yAxis.axisMaximum = 100
        
        let xAxis = chartView.xAxis
        xAxis.labelPosition = .bottom
        xAxis.labelFont = .boldSystemFont(ofSize: 12)
        xAxis.setLabelCount(6, force: false)
        //xAxis.labelRotationAngle = -45
        //xAxis.labelTextColor = .white
        //xAxis.axisLineColor = .white

        return chartView
    }()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        overrideUserInterfaceStyle = .dark

        // Picker
        self.picker.dataSource = self
        self.picker.delegate = self
        
        let last = (pickerData?.count ?? 2)-1
        //print(last)
        self.picker.selectRow(last, inComponent: 0, animated: true)
        
        // LineChart
        view.addSubview(lineChartView)
        //lineChartView.centerInSuperview(offset: CGPoint(x: 0, y:50), usingSafeArea: true)
        lineChartView.bottom(to: view, offset:-100)
        lineChartView.width(to: view)
        lineChartView.heightToWidth(of: view, multiplier: 1.2)
        lineChartView.notifyDataSetChanged()
      
        setData()
        
        // Refresh Graph every x seconds (for live view)
        Timer.scheduledTimer(timeInterval: 3, target: self, selector: #selector(ThirdViewController.setData), userInfo: nil, repeats: true)
    }
    
    override func viewDidAppear(_ animated: Bool) {
        print("viewDidAppear")
        picker.reloadAllComponents()

    }

    
    @objc func setData(){
        
        // Create ChartDataSet SET1
        let spo2csvwerte = csvwerteausgeben(pickerAuswahl: pickerAuswahl!, Spalte: spalte_spo2)
        let spo2 = spo2csvwerte.map { (value) -> Double in
            return Double(value)!}
        
        // 1 - creating an array of data entries
        var set1values : [ChartDataEntry] = [ChartDataEntry]()
        for i in 0 ..< spo2.count {
            set1values.append(ChartDataEntry(x: Double(i+1), y: spo2[i]))
        }
        
        // Create ChartDataSet SET2
        let bpmcsvwerte = csvwerteausgeben(pickerAuswahl: pickerAuswahl!, Spalte: spalte_bpm)
        let bpm = bpmcsvwerte.map { (value) -> Double in
            return Double(value)!}

        // 2 - creating an array of data entries
        var set2values : [ChartDataEntry] = [ChartDataEntry]()
        for i in 0 ..< bpm.count {
            set2values.append(ChartDataEntry(x: Double(i+1), y: bpm[i]))
        }
        
        // Set 1 = SPO2
        let set1 = LineChartDataSet(entries: set1values, label: "SpO2")
        set1.drawCirclesEnabled = false
        //set1.mode = .cubicBezier
        set1.cubicIntensity = 0.2
        set1.lineWidth = 1.5
        set1.setColor(myblue)
        set1.notifyDataSetChanged()
        
        // Set 2 = BPM
        let set2 = LineChartDataSet(entries: set2values, label: "BPM")
        set2.drawCirclesEnabled = false
        //set1.mode = .cubicBezier
        set2.cubicIntensity = 0.2
        set2.lineWidth = 1.5
        set2.setColor(myred)
        set2.notifyDataSetChanged()
        
        let data = LineChartData(dataSets: [set1,set2])
        data.setDrawValues(false)
        lineChartView.data = data
        lineChartView.setVisibleXRangeMaximum(3000)
        lineChartView.setVisibleXRangeMinimum(10)
        
        // x- Achsen Werte ersetzen
        let customFormatter = CustomFormatter()
        let timecsvwerte = csvwerteausgeben(pickerAuswahl: pickerAuswahl!, Spalte: spalte_time)
        // x-Achse: Zeiten kürzen, Sekunden entfernen
        var shorttimecsvwerte: [String] = []
        for i in 0 ..< timecsvwerte.count {
            let dropped = timecsvwerte[i].dropLast(3)
            shorttimecsvwerte.append(String(dropped))
        }
        customFormatter.labels = shorttimecsvwerte
        lineChartView.xAxis.valueFormatter = customFormatter
    }
    

    // x-Achsen Werte ersetzen
    final class CustomFormatter: IAxisValueFormatter {
        var labels: [String] = []
        func stringForValue(_ value: Double, axis: AxisBase?) -> String {
            let count = self.labels.count
            guard let axis = axis, count > 0 else {
                return ""
            }
            let factor = axis.axisMaximum / Double(count)
            let index = Int((value / factor).rounded())
            if index >= 0 && index < count {
                return self.labels[index]
            }
            return ""
        }
    }

}



//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - ALARMGRENZE VIEW CONTROLLER  - - - - - - -- - - - - - - - - ///
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
class FourthViewController: UIViewController, UIPickerViewDelegate, UIPickerViewDataSource {
    
    @IBOutlet weak var alarmgrenzeLabel: UILabel!
    @IBAction func alarmgrenzebutton(_ sender: Any) {
        print("Daten versenden Button pressed")
        let value = String(pickerAuswahl!).asciiValues
        alarmgrenze_last = alarmgrenze
        alarmgrenze = String(pickerAuswahl!).asciiValues
        print(value)
        // Ausgewählten Wert an WakeMe senden
        StatusController.sendData(datasource: value)
    }
    

    // PickerVerlauf initiieren
    lazy var pickerAuswahl = pickerData.last
    
    // CSV Datei Picker
    @IBOutlet weak var picker: UIPickerView!
    lazy var pickerData = [80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]
    
    func numberOfComponents(in pickerView: UIPickerView) -> Int {
        return 1
    }
    func pickerView(_ pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return pickerData.count
    }
    func pickerView(_ pickerView: UIPickerView, viewForRow row: Int, forComponent component: Int, reusing view: UIView?) -> UIView {
        var label = UILabel()
            if let v = view {
                label = v as! UILabel
            }
            label.text =  String(pickerData[row])
            label.textAlignment = .center
            return label
        }
    
     func pickerView(_ pickerView: UIPickerView, didSelectRow row: Int, inComponent  component: Int) {
        pickerAuswahl = pickerData[row] as Int
        print(pickerAuswahl ?? "0")
     }
    
    override func reloadInputViews() {
        let alarmgrenze = alarmgrenzearr.last
        alarmgrenzeLabel.text = String(alarmgrenze ?? "0")
        //print("wie hoft")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        // datePicker
        self.picker.dataSource = self
        self.picker.delegate = self
        picker.reloadAllComponents()
        let last = (pickerData.count)-1
        self.picker.selectRow(last, inComponent: 0, animated: true)

        // UI
        overrideUserInterfaceStyle = .dark
        
        // Refresh Graph every x seconds (for live view)
        Timer.scheduledTimer(timeInterval: 0.1, target: self, selector: #selector(FourthViewController.reloadInputViews), userInfo: nil, repeats: true)
    }
    
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
//  - - - - - - - - - - - - - - DESIGN - - - - - - - - - - - - - - - - -- - - - - - - - - - ///
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ///
extension UILabel {
    @IBInspectable
    var rotation: Int {
        get {
            return 0
        } set {
            let radians = CGFloat(CGFloat(Double.pi) * CGFloat(newValue) / CGFloat(180.0))
            self.transform = CGAffineTransform(rotationAngle: radians)
        }
    }
}

// Design
extension UILabel {
   func UILableTextShadow(){
      self.layer.masksToBounds = false
      self.layer.shadowOffset = CGSize(width: 1, height: 1)
      self.layer.rasterizationScale = UIScreen.main.scale
      self.layer.shadowRadius = 4.0
    self.layer.shadowOpacity = 1
    self.layer.shadowColor = UIColor.black.cgColor
    
   }
}

// Design
extension UILabel {
   func UILabelCorner(){
    self.layer.cornerRadius = 20
   }
}

// ASCII
extension StringProtocol {
    var asciiValues: [UInt8] { compactMap(\.asciiValue) }
}

// Audio
//Update system volume if Alarm
extension MPVolumeView {
    static func setVolume(_ volume: Float) {
        let volumeView = MPVolumeView()
        let slider = volumeView.subviews.first(where: { $0 is UISlider }) as? UISlider

        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + 0.01) {
            slider?.value = volume
        }
    }
}

// Vibration
extension UIDevice {
    static func vibrate() {
        AudioServicesPlaySystemSound(kSystemSoundID_Vibrate)
    }
}
