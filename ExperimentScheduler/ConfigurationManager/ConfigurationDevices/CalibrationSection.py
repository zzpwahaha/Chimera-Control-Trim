import re
from collections import OrderedDict
from ConfigurationManager.ConfigurationSectionClass.ConfigurationSection import ConfigurationSection
from ConfigurationManager.ConfigurationSectionClass.ConfigurationItems import ConfigurationItems
from ConfigurationManager.ConfigurationDevices.CalibrationItem import CalibrationItem

class CalibrationSection(ConfigurationSection):
    def __init__(self, section_name, data_chunk):
        self.PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")
        self.section_name = section_name
        self.parameters = OrderedDict()
        self.calibrations = []
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Auto Cal Checked: \*/\s*(\d+)', data_chunk):
            self.add_parameter("Auto Cal Checked: ", _match.group(1))
        else:
            raise ValueError(r"Expecting /*Auto Cal Checked: */ in CalibrationSection but is not found in .Config")
        num_cal = 0
        if _match := re.search(r'/\*Calibration Number: \*/\s*(\d+)', data_chunk):
            num_cal = int(_match.group(1))
        else:
            raise ValueError(r"Expecting /*Calibration Number: */ in CalibrationSection but is not found in .Config")

        # Use regex to capture both the Range delimiter and its content
        split_chunks = re.findall(r'(/\*Calibration Name:.*?)(?=(/\*Calibration Name|\Z))', data_chunk, re.DOTALL)
        for split_chunk in split_chunks:
            cal_items = CalibrationItem(split_chunk[0].strip())
            self.calibrations.append(cal_items)
        assert num_cal==len(self.calibrations), "len of calibration should match the one in the config."
            

    def print(self):
        """Returns the string representation of the section."""
        strs = [f"{self.section_name}", 
                f"/*Auto Cal Checked: */ {self.parameters['Auto Cal Checked: ']}",
                f"/*Calibration Number: */ {len(self.calibrations)}"]
        for cal in self.calibrations:
            strs.append(cal.print())
        strs.append(f"END_{self.section_name}")

        return "\n".join(strs)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()

if __name__ == "__main__":
    data_chunk = """CALIBRATION_MANAGER
/*Auto Cal Checked: */ 0
/*Calibration Number: */ 13
/*Calibration Name: */ mot_pwr
/*Analog Input Chanel: */ mot_topleft
/*Analog Output Control Chanel: */ scimotamp
/*Control Values: */ [ 0 9.5 ] 21
/*Calibration Active: */ 1
/*TTL Config Size: */ 3
/*ttl config: */ scimottrg xyzmotshutter xymotshutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.1280947 1.96815308 2.52588579 4.23033708 5.43539201 6.98553112 7.94554502 9.1813819 9.49844195 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */21
/*Control Vals: */0. 0.475 0.95 1.425 1.9 2.375 2.85 3.325 3.8 4.275 4.75 5.225 5.7 6.175 6.65 7.125 7.6 8.075 8.55 9.025 9.5 
/*Number of Result Vals: */21
/*Result Vals: */0.00115909 0.04288522 0.13514756 0.28242805 0.46993439 0.70163335 0.97871822 1.28518815 1.61281838 1.95652699 2.32180486 2.71066317 3.08333934 3.48411451 3.89698538 4.26684347 4.62746361 4.97185095 5.24654378 5.54307505 5.87487289 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.14752011 1.78529475 2.52244282 4.06347178 5.37460955 6.82747142 8.04031598 8.94524798 9.4960773 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.19 0.38 0.57 0.76 0.95 1.14 1.33 1.52 1.71 1.9 2.09 2.28 2.47 2.66 2.85 3.04 3.23 3.42 3.61 3.8 3.99 4.18 4.37 4.56 4.75 4.94 5.13 5.32 5.51 5.7 5.89 6.08 6.27 6.46 6.65 6.84 7.03 7.22 7.41 7.6 7.79 7.98 8.17 8.36 8.55 8.74 8.93 9.12 9.31 9.5 
/*Number of Result Vals: */51
/*Result Vals: */0.00581622 0.02149724 0.04750755 0.08371471 0.12984649 0.18564287 0.25083834 0.32567522 0.40938444 0.50101993 0.60081057 0.70818751 0.82226081 0.94506119 1.07440901 1.20953032 1.3509476 1.49671621 1.64852809 1.80462905 1.96596454 2.12842006 2.29489242 2.46584796 2.64015381 2.80560381 2.98246284 3.15746574 3.34192511 3.5210596 3.70726218 3.88989837 4.07671316 4.26067141 4.43685232 4.6103177 4.77749138 4.94642781 5.12060427 5.28193487 5.44286874 5.59304361 5.74451369 5.88602557 6.04572222 6.19634449 6.34578997 6.47960936 6.60593036 6.73294839 6.85042146 
/*Calibration Name: */ prb_pwr
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbamp
/*Control Values: */ [ 0 5.5 ] 21
/*Calibration Active: */ 1
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbcoolshutter 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbrepamp 0 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.12732991 1.45106012 1.73916443 2.92479947 3.54655826 4.49632341 4.92129946 5.35203163 5.499508 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */21
/*Control Vals: */0. 0.275 0.55 0.825 1.1 1.375 1.65 1.925 2.2 2.475 2.75 3.025 3.3 3.575 3.85 4.125 4.4 4.675 4.95 5.225 5.5 
/*Number of Result Vals: */21
/*Result Vals: */-0.00238166 0.03025666 0.12149724 0.25882504 0.46967254 0.72268075 1.02664998 1.39012055 1.77811396 2.22827113 2.69965575 3.21589526 3.75421125 4.33172765 4.93077303 5.54324229 6.18207953 6.82763756 7.48821986 8.1723014 8.86490616 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.17836913 1.45824111 1.81515266 2.95763929 3.62746195 4.56771994 5.00027225 5.44140903 5.5961249 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.112 0.224 0.336 0.448 0.56 0.672 0.784 0.896 1.008 1.12 1.232 1.344 1.456 1.568 1.68 1.792 1.904 2.016 2.128 2.24 2.352 2.464 2.576 2.688 2.8 2.912 3.024 3.136 3.248 3.36 3.472 3.584 3.696 3.808 3.92 4.032 4.144 4.256 4.368 4.48 4.592 4.704 4.816 4.928 5.04 5.152 5.264 5.376 5.488 5.6 
/*Number of Result Vals: */51
/*Result Vals: */0.00164556 0.00512528 0.01916746 0.04363842 0.07869015 0.12406629 0.17956847 0.24514664 0.32106326 0.40598773 0.50097537 0.60483169 0.71866634 0.84252693 0.97432112 1.11433393 1.26182501 1.41901791 1.58554399 1.75882199 1.93694937 2.12460829 2.31920347 2.52649983 2.73903989 2.9509537 3.17056368 3.39802057 3.62643208 3.87171972 4.10464431 4.35717704 4.60771813 4.86438185 5.12772423 5.38823817 5.66080142 5.93436689 6.21093173 6.48328074 6.77075716 7.0607178 7.35328288 7.63962951 7.93194311 8.22239204 8.51657094 8.81170629 9.10543413 9.40477615 9.70027528 
/*Calibration Name: */ prb_rep_pwr
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0 6.5 ] 51
/*Calibration Active: */ 1
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 5 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.14283758 1.25793383 1.62466895 2.70275033 3.46559511 4.60048687 5.27533456 6.05125056 6.50320512 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.13 0.26 0.39 0.52 0.65 0.78 0.91 1.04 1.17 1.3 1.43 1.56 1.69 1.82 1.95 2.08 2.21 2.34 2.47 2.6 2.73 2.86 2.99 3.12 3.25 3.38 3.51 3.64 3.77 3.9 4.03 4.16 4.29 4.42 4.55 4.68 4.81 4.94 5.07 5.2 5.33 5.46 5.59 5.72 5.85 5.98 6.11 6.24 6.37 6.5 
/*Number of Result Vals: */51
/*Result Vals: */0.00178961 0.00218268 0.00395093 0.00595233 0.00910611 0.0137199 0.01884396 0.0244789 0.03109043 0.03904355 0.04695822 0.05544542 0.06515519 0.07503281 0.08506241 0.09600757 0.10782922 0.11942991 0.1314774 0.14437147 0.15713004 0.17012848 0.18365307 0.19709708 0.20975982 0.22312143 0.23688101 0.24990081 0.26340342 0.27660207 0.290253 0.30715537 0.31771111 0.33011994 0.34105167 0.35496872 0.36803125 0.38010071 0.39246315 0.40362377 0.41641285 0.42854457 0.43841792 0.44818018 0.45856075 0.46826136 0.47573412 0.4829664 0.49061739 0.50009461 0.50703574 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.16551235 1.31966252 1.64163303 2.75659833 3.55986459 4.65406354 5.50949838 6.34068982 6.98589698 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.14 0.28 0.42 0.56 0.7 0.84 0.98 1.12 1.26 1.4 1.54 1.68 1.82 1.96 2.1 2.24 2.38 2.52 2.66 2.8 2.94 3.08 3.22 3.36 3.5 3.64 3.78 3.92 4.06 4.2 4.34 4.48 4.62 4.76 4.9 5.04 5.18 5.32 5.46 5.6 5.74 5.88 6.02 6.16 6.3 6.44 6.58 6.72 6.86 7. 
/*Number of Result Vals: */51
/*Result Vals: */0.00169134 0.00200873 0.00368236 0.006816 0.01135838 0.01734611 0.02458632 0.03295511 0.04261849 0.05321146 0.06509354 0.07768975 0.09142064 0.10524857 0.12070009 0.13612293 0.15211585 0.16855495 0.18489578 0.20267708 0.22009339 0.23835261 0.25630421 0.27437483 0.2925602 0.31165441 0.32913236 0.34839076 0.36574664 0.38440077 0.40213324 0.4195587 0.43732353 0.45274697 0.46953642 0.48551225 0.50140141 0.51467452 0.52952361 0.54260994 0.55533433 0.56888028 0.58177252 0.5942552 0.6041847 0.61135655 0.6215717 0.63261208 0.6392822 0.64779504 0.65363018 
/*Calibration Name: */ prb_rep_pwr_attenuate
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0 7 ] 51
/*Calibration Active: */ 0
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 5 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.15884738 1.3531565 1.71659576 2.85689443 3.6417267 4.79850628 5.62242502 6.41687049 7.00047572 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.14 0.28 0.42 0.56 0.7 0.84 0.98 1.12 1.26 1.4 1.54 1.68 1.82 1.96 2.1 2.24 2.38 2.52 2.66 2.8 2.94 3.08 3.22 3.36 3.5 3.64 3.78 3.92 4.06 4.2 4.34 4.48 4.62 4.76 4.9 5.04 5.18 5.32 5.46 5.6 5.74 5.88 6.02 6.16 6.3 6.44 6.58 6.72 6.86 7. 
/*Number of Result Vals: */51
/*Result Vals: */0.00124332 0.00160588 0.00309885 0.00560442 0.00907437 0.01360027 0.01908628 0.02567461 0.03271767 0.04077517 0.04964202 0.0591522 0.06946318 0.08021973 0.09183081 0.10389782 0.11644459 0.12954253 0.14296335 0.15659291 0.17015046 0.18463454 0.19953124 0.21399823 0.2289169 0.24327342 0.25859371 0.27353252 0.28802881 0.3024189 0.31724784 0.33126194 0.34472671 0.35851863 0.3713126 0.38526566 0.39782159 0.40991913 0.42177984 0.43333048 0.44546342 0.45783319 0.4681344 0.47730216 0.48813807 0.49626148 0.5047853 0.51247658 0.52002808 0.52710593 0.53365459 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 0
/*Calibration Coefficients: */ "!#EMPTY_STRING#!"
/*Calibration BSpline order: */ 0
/*Calibration BSpline breakpoints: */ 0
/*Number of Control Vals: */0
/*Control Vals: */"!#EMPTY_STRING#!"
/*Number of Result Vals: */0
/*Result Vals: */"!#EMPTY_STRING#!"
/*Calibration Name: */ prb_rep_pwr_9Vtotal
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0 7 ] 51
/*Calibration Active: */ 0
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 9 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.16162175 1.35592638 1.72955099 2.85184554 3.69337481 4.8275317 5.65533349 6.44672183 7.00409688 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.14 0.28 0.42 0.56 0.7 0.84 0.98 1.12 1.26 1.4 1.54 1.68 1.82 1.96 2.1 2.24 2.38 2.52 2.66 2.8 2.94 3.08 3.22 3.36 3.5 3.64 3.78 3.92 4.06 4.2 4.34 4.48 4.62 4.76 4.9 5.04 5.18 5.32 5.46 5.6 5.74 5.88 6.02 6.16 6.3 6.44 6.58 6.72 6.86 7. 
/*Number of Result Vals: */51
/*Result Vals: */0.00131474 0.00218818 0.00546037 0.01115452 0.01917966 0.02952605 0.04192083 0.05664663 0.07341288 0.09199255 0.11237892 0.13436262 0.15802423 0.18286203 0.20912931 0.23675771 0.26499649 0.2943083 0.32529801 0.35668752 0.38936918 0.42160039 0.45384625 0.48659078 0.52103519 0.5538786 0.58650594 0.62009705 0.65318094 0.68710105 0.72083377 0.75269387 0.78554949 0.81805353 0.8490994 0.87981628 0.90913114 0.93779656 0.96524796 0.99377422 1.02023927 1.04686789 1.07142064 1.09607898 1.11988098 1.14108707 1.16059145 1.17853572 1.19522446 1.21327372 1.22971648 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 0
/*Calibration Coefficients: */ "!#EMPTY_STRING#!"
/*Calibration BSpline order: */ 0
/*Calibration BSpline breakpoints: */ 0
/*Number of Control Vals: */0
/*Control Vals: */"!#EMPTY_STRING#!"
/*Number of Result Vals: */0
/*Result Vals: */"!#EMPTY_STRING#!"
/*Calibration Name: */ prb_rep_pwr_2p2Vtotal
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0.1 7.5] 51
/*Calibration Active: */ 1
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 2.2 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.18581703 1.27734277 1.7441923 2.81756746 3.70524093 4.85238263 5.81813359 6.74601573 7.47226585 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0.1 0.248 0.396 0.544 0.692 0.84 0.988 1.136 1.284 1.432 1.58 1.728 1.876 2.024 2.172 2.32 2.468 2.616 2.764 2.912 3.06 3.208 3.356 3.504 3.652 3.8 3.948 4.096 4.244 4.392 4.54 4.688 4.836 4.984 5.132 5.28 5.428 5.576 5.724 5.872 6.02 6.168 6.316 6.464 6.612 6.76 6.908 7.056 7.204 7.352 7.5 
/*Number of Result Vals: */51
/*Result Vals: */0.00169622 0.00215766 0.00279489 0.00364696 0.00482498 0.00614765 0.00762597 0.00987945 0.01197913 0.01430342 0.01667287 0.01933714 0.02206854 0.0250856 0.02818812 0.03126316 0.03449507 0.03782647 0.04124821 0.04509171 0.04875027 0.05222571 0.05570544 0.05940306 0.06283822 0.06650288 0.07009247 0.07358074 0.07708243 0.08047792 0.08419507 0.08734458 0.0902945 0.09307047 0.09628162 0.09934263 0.10217902 0.1045735 0.107687 0.10944731 0.11254189 0.1149913 0.11620167 0.11827937 0.12015015 0.12250313 0.12431959 0.12604083 0.1269808 0.12864895 0.12961028 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.18581703 1.27734277 1.7441923 2.81756746 3.70524093 4.85238263 5.81813359 6.74601573 7.47226585 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0.1 0.248 0.396 0.544 0.692 0.84 0.988 1.136 1.284 1.432 1.58 1.728 1.876 2.024 2.172 2.32 2.468 2.616 2.764 2.912 3.06 3.208 3.356 3.504 3.652 3.8 3.948 4.096 4.244 4.392 4.54 4.688 4.836 4.984 5.132 5.28 5.428 5.576 5.724 5.872 6.02 6.168 6.316 6.464 6.612 6.76 6.908 7.056 7.204 7.352 7.5 
/*Number of Result Vals: */51
/*Result Vals: */0.00169622 0.00215766 0.00279489 0.00364696 0.00482498 0.00614765 0.00762597 0.00987945 0.01197913 0.01430342 0.01667287 0.01933714 0.02206854 0.0250856 0.02818812 0.03126316 0.03449507 0.03782647 0.04124821 0.04509171 0.04875027 0.05222571 0.05570544 0.05940306 0.06283822 0.06650288 0.07009247 0.07358074 0.07708243 0.08047792 0.08419507 0.08734458 0.0902945 0.09307047 0.09628162 0.09934263 0.10217902 0.1045735 0.107687 0.10944731 0.11254189 0.1149913 0.11620167 0.11827937 0.12015015 0.12250313 0.12431959 0.12604083 0.1269808 0.12864895 0.12961028 
/*Calibration Name: */ prb_rep_pwr_1p5Vtotal_atten
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0 7 ] 16
/*Calibration Active: */ 0
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 1.5 
/*Data Point Average Number: */ 1000
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 8
/*Calibration Coefficients: */ 0.14317205 1.56288735 2.03993361 3.29661193 4.1376599 5.30095954 6.14854012 7.0013696 
/*Calibration BSpline order: */ 4
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */16
/*Control Vals: */0. 0.46666667 0.93333333 1.4 1.86666667 2.33333333 2.8 3.26666667 3.73333333 4.2 4.66666667 5.13333333 5.6 6.06666667 6.53333333 7. 
/*Number of Result Vals: */16
/*Result Vals: */0.0012476 0.0017716 0.00362499 0.00645863 0.0103824 0.01481429 0.01986724 0.02538224 0.03079867 0.03631153 0.04151738 0.04625416 0.05070467 0.05439985 0.05735191 0.05972777 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 8
/*Calibration Coefficients: */ 0.50142808 1.41244098 2.17765358 3.23073138 4.21371702 5.29512982 6.21730479 6.99042161 
/*Calibration BSpline order: */ 4
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */16
/*Control Vals: */0.5 0.93333333 1.36666667 1.8 2.23333333 2.66666667 3.1 3.53333333 3.96666667 4.4 4.83333333 5.26666667 5.7 6.13333333 6.56666667 7. 
/*Number of Result Vals: */16
/*Result Vals: */0.00144742 0.00178503 0.00213195 0.00280778 0.00337527 0.00414701 0.00496117 0.00573305 0.00650418 0.00734062 0.00802194 0.00874882 0.00938627 0.00986663 0.01036988 0.01071162 
/*Calibration Name: */ prb_rep_pwr_6Vtotal
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ prbrepamp
/*Control Values: */ [ 0 7 ] 51
/*Calibration Active: */ 0
/*TTL Config Size: */ 3
/*ttl config: */ prbtrg prbshutter prbreptrg 
/*Analog Output Config Size: */ 1
/*Analog Output Config: */ prbamp 6 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.16524316 1.35608809 1.72954905 2.85969843 3.66882114 4.8172256 5.60751144 6.44570221 6.9982517 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.14 0.28 0.42 0.56 0.7 0.84 0.98 1.12 1.26 1.4 1.54 1.68 1.82 1.96 2.1 2.24 2.38 2.52 2.66 2.8 2.94 3.08 3.22 3.36 3.5 3.64 3.78 3.92 4.06 4.2 4.34 4.48 4.62 4.76 4.9 5.04 5.18 5.32 5.46 5.6 5.74 5.88 6.02 6.16 6.3 6.44 6.58 6.72 6.86 7. 
/*Number of Result Vals: */51
/*Result Vals: */0.00132145 0.00163457 0.00351085 0.00667928 0.011489 0.0173162 0.0243733 0.03273537 0.04209723 0.05264809 0.06416578 0.07678335 0.08992279 0.10422926 0.11910886 0.13483505 0.15085421 0.1678927 0.1852443 0.20335093 0.22132328 0.24016968 0.25926084 0.27831599 0.29715323 0.31641652 0.33512986 0.35453108 0.3737846 0.39320963 0.41241005 0.43082919 0.44933439 0.46758507 0.48482498 0.50226325 0.51893918 0.53645009 0.55231849 0.56787255 0.58298166 0.59774163 0.6117362 0.62544816 0.63628346 0.64840358 0.6599939 0.67043062 0.68097293 0.6892349 0.69862056 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.16383195 1.39121165 1.73657577 2.89001652 3.67841249 4.82336513 5.5950805 6.40368975 6.99720775 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.14 0.28 0.42 0.56 0.7 0.84 0.98 1.12 1.26 1.4 1.54 1.68 1.82 1.96 2.1 2.24 2.38 2.52 2.66 2.8 2.94 3.08 3.22 3.36 3.5 3.64 3.78 3.92 4.06 4.2 4.34 4.48 4.62 4.76 4.9 5.04 5.18 5.32 5.46 5.6 5.74 5.88 6.02 6.16 6.3 6.44 6.58 6.72 6.86 7. 
/*Number of Result Vals: */51
/*Result Vals: */0.00140049 0.0020246 0.00421628 0.00779107 0.01300745 0.01961303 0.02788949 0.03752251 0.04862331 0.06098453 0.07449858 0.08933851 0.10484161 0.1218247 0.13961989 0.15808679 0.17749474 0.19764641 0.21879711 0.23987701 0.26121769 0.28332408 0.30606494 0.32927915 0.35230079 0.37551012 0.39867718 0.42232856 0.44544939 0.46711753 0.49140614 0.5135052 0.53543504 0.55660115 0.57853511 0.60058794 0.62198843 0.64203879 0.66184317 0.68146489 0.69879971 0.715506 0.73207205 0.74813669 0.76386334 0.77758919 0.79190725 0.80439833 0.81439924 0.82402493 0.83485214 
/*Calibration Name: */ d1lgm_pwr
/*Analog Input Chanel: */ mot_topright
/*Analog Output Control Chanel: */ d1lgmamp
/*Control Values: */ [ 0 8.0 ] 21
/*Calibration Active: */ 1
/*TTL Config Size: */ 3
/*ttl config: */ d1lgmtrg d1lgmshutter xymotshutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.11581644 1.47789808 1.81921672 3.07649913 3.88078802 5.22400028 5.78081705 7.33242428 7.99298921 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */21
/*Control Vals: */0. 0.4 0.8 1.2 1.6 2. 2.4 2.8 3.2 3.6 4. 4.4 4.8 5.2 5.6 6. 6.4 6.8 7.2 7.6 8. 
/*Number of Result Vals: */21
/*Result Vals: */0.00025636 0.00551408 0.02289621 0.04835047 0.08378857 0.12450697 0.17594653 0.22860012 0.28782556 0.34403943 0.40446547 0.46359996 0.52249825 0.57834101 0.63187048 0.67520188 0.71018647 0.73656423 0.75921873 0.7864333 0.80838954 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.14135373 1.42418236 1.8682423 3.06543155 3.96425659 5.17233016 6.08867354 6.99740453 7.96548693 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.16 0.32 0.48 0.64 0.8 0.96 1.12 1.28 1.44 1.6 1.76 1.92 2.08 2.24 2.4 2.56 2.72 2.88 3.04 3.2 3.36 3.52 3.68 3.84 4. 4.16 4.32 4.48 4.64 4.8 4.96 5.12 5.28 5.44 5.6 5.76 5.92 6.08 6.24 6.4 6.56 6.72 6.88 7.04 7.2 7.36 7.52 7.68 7.84 8. 
/*Number of Result Vals: */51
/*Result Vals: */0.00046937 0.00273568 0.00770043 0.01558275 0.02606891 0.03923093 0.05503037 0.0731077 0.09342875 0.11642567 0.14137638 0.16833399 0.19704642 0.22771264 0.26024598 0.29372967 0.32796716 0.36383251 0.40142033 0.43933714 0.47868831 0.51755486 0.55735038 0.59574938 0.6361388 0.67696707 0.71730033 0.75854427 0.79765374 0.8336259 0.87148595 0.91065828 0.94591327 0.9829902 1.01963317 1.05209449 1.08508988 1.11677358 1.14643941 1.17483139 1.19845882 1.22467544 1.24822901 1.27073153 1.28804163 1.30569231 1.32058046 1.3360857 1.35116611 1.36157048 1.37011872 
/*Calibration Name: */ d1lgm_prb_pwr
/*Analog Input Chanel: */ prb_pickoff
/*Analog Output Control Chanel: */ d1lgmamp
/*Control Values: */ [ 0 8.0 ] 21
/*Calibration Active: */ 1
/*TTL Config Size: */ 2
/*ttl config: */ d1lgmtrg d1lgmprbshutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.11920406 1.47393721 1.83957867 3.04759561 3.95560693 5.08964882 6.11650763 6.85555875 7.96578322 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */21
/*Control Vals: */0. 0.4 0.8 1.2 1.6 2. 2.4 2.8 3.2 3.6 4. 4.4 4.8 5.2 5.6 6. 6.4 6.8 7.2 7.6 8. 
/*Number of Result Vals: */21
/*Result Vals: */0.00743065 0.05990661 0.23551866 0.49230201 0.85438826 1.27798456 1.78070254 2.31634266 2.90534013 3.49937986 4.10658406 4.70416211 5.29531907 5.84426038 6.36708762 6.83946287 7.24385693 7.60814844 7.8767394 8.09589709 8.2223603 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.15844124 1.4812002 1.93795441 3.14152905 4.09908464 5.24874896 6.28307763 7.03594349 7.88336884 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.16 0.32 0.48 0.64 0.8 0.96 1.12 1.28 1.44 1.6 1.76 1.92 2.08 2.24 2.4 2.56 2.72 2.88 3.04 3.2 3.36 3.52 3.68 3.84 4. 4.16 4.32 4.48 4.64 4.8 4.96 5.12 5.28 5.44 5.6 5.76 5.92 6.08 6.24 6.4 6.56 6.72 6.88 7.04 7.2 7.36 7.52 7.68 7.84 8. 
/*Number of Result Vals: */51
/*Result Vals: */0.00176824 0.01443037 0.04692526 0.09897336 0.17009369 0.25951231 0.36716697 0.49241127 0.63227943 0.79352275 0.96323984 1.15337565 1.35152501 1.56459487 1.78684164 2.02613361 2.26896634 2.52404859 2.78127689 3.05362957 3.3239143 3.59837092 3.88771386 4.16956511 4.45839839 4.74943815 5.03433149 5.31756767 5.60971221 5.88691061 6.17627796 6.44095462 6.70912809 6.98170354 7.23881649 7.48366771 7.73624439 7.98682455 8.20475112 8.40115238 8.5965569 8.79798029 9.00787378 9.19366008 9.35158666 9.52703757 9.6623719 9.78175787 9.91045503 10. 10. 
/*Calibration Name: */ op_pwr
/*Analog Input Chanel: */ op_pickoff
/*Analog Output Control Chanel: */ opamp
/*Control Values: */ [ 0 4.0 ] 41
/*Calibration Active: */ 1
/*TTL Config Size: */ 2
/*ttl config: */ optrg opshutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.10525251 1.04971084 1.34033046 2.19286614 2.65639402 3.3124429 3.62585084 3.89359734 3.99853604 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */41
/*Control Vals: */0. 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1. 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2. 2.1 2.2 2.3 2.4 2.5 2.6 2.7 2.8 2.9 3. 3.1 3.2 3.3 3.4 3.5 3.6 3.7 3.8 3.9 4. 
/*Number of Result Vals: */41
/*Result Vals: */-0.00487869 0.00123112 0.01261635 0.03161657 0.05718192 0.08871181 0.12894742 0.17206519 0.22185492 0.27890683 0.34028993 0.40849696 0.48191046 0.56209418 0.64886196 0.74149785 0.83831965 0.94046327 1.04439222 1.15679434 1.27243751 1.39322184 1.51689077 1.65096835 1.78906888 1.93157933 2.07802057 2.22343944 2.37912168 2.54441481 2.70667684 2.85970153 3.02136723 3.18649312 3.35902951 3.5358269 3.71516587 3.89224036 4.08409925 4.27470809 4.46312143 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.1014642 1.01993899 1.32861961 2.14813178 2.64755533 3.29485262 3.59950069 3.89049761 3.99761737 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */51
/*Control Vals: */0. 0.08 0.16 0.24 0.32 0.4 0.48 0.56 0.64 0.72 0.8 0.88 0.96 1.04 1.12 1.2 1.28 1.36 1.44 1.52 1.6 1.68 1.76 1.84 1.92 2. 2.08 2.16 2.24 2.32 2.4 2.48 2.56 2.64 2.72 2.8 2.88 2.96 3.04 3.12 3.2 3.28 3.36 3.44 3.52 3.6 3.68 3.76 3.84 3.92 4. 
/*Number of Result Vals: */51
/*Result Vals: */0.00084536 0.00863552 0.02296091 0.04381603 0.07120762 0.10511429 0.1454091 0.1923661 0.24567461 0.30530045 0.37167638 0.44383618 0.5221308 0.60638752 0.69706656 0.79356731 0.8951915 1.00361766 1.11700797 1.23565111 1.35874142 1.48918668 1.62258309 1.76150273 1.90582354 2.05494858 2.20794946 2.36537431 2.53047639 2.6943083 2.86313303 3.03913083 3.21773247 3.4040315 3.59223609 3.78151189 3.97692984 4.17532517 4.38054201 4.58272469 4.79206641 5.00531266 5.22229621 5.43914243 5.6544554 5.87881649 6.09724357 6.32378979 6.55276955 6.77794855 7.0029725 
/*Calibration Name: */ ryd420_pwr
/*Analog Input Chanel: */ ryd420_pickoff
/*Analog Output Control Chanel: */ ryd420amp
/*Control Values: */ [ 1 7.5 ] 21
/*Calibration Active: */ 1
/*TTL Config Size: */ 2
/*ttl config: */ ryd420trg ryd420shutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 1.19045385 3.04780884 2.94084929 4.25402758 4.67202306 5.6889803 6.34467808 6.90721651 7.49616351 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */21
/*Control Vals: */1. 1.325 1.65 1.975 2.3 2.625 2.95 3.275 3.6 3.925 4.25 4.575 4.9 5.225 5.55 5.875 6.2 6.525 6.85 7.175 7.5 
/*Number of Result Vals: */21
/*Result Vals: */-0.00994156 -0.0096646 -0.0090315 -0.00788995 -0.00606723 -0.00337794 0.00033174 0.00512848 0.01088382 0.01760674 0.02481399 0.03242744 0.04014527 0.04759224 0.05502441 0.06164678 0.0679812 0.07346355 0.07919492 0.08284066 0.0857419 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 2.00429609 2.98127155 3.58771369 4.4383004 5.26735904 6.07713469 6.93252064 7.50101542 8.49375094 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */11
/*Control Vals: */2. 2.65 3.3 3.95 4.6 5.25 5.9 6.55 7.2 7.85 8.5 
/*Number of Result Vals: */11
/*Result Vals: */-0.00761223 -0.00282006 0.00578097 0.01851619 0.03457945 0.05267663 0.07071566 0.08725959 0.10010224 0.10894421 0.11362148 
/*Calibration Name: */ ryd1013_pwr
/*Analog Input Chanel: */ ryd1013_pickoff
/*Analog Output Control Chanel: */ ryd1013amp
/*Control Values: */ [ 0 10 ] 41
/*Calibration Active: */ 1
/*TTL Config Size: */ 2
/*ttl config: */ ryd1013trg ryd1013shutter 
/*Analog Output Config Size: */ 0
/*Analog Output Config: */ 
/*Data Point Average Number: */ 500
/*Use Agilent: */0
/*Which Agilent: */Cryo Siglent
/*Which Agilent Channel: */1
/*Recent Calibration Result:*/
/*Number of Calibration Coefficients: */ 9
/*Calibration Coefficients: */ 0.20803665 1.57540007 2.09730471 3.11780886 4.26767981 4.68238255 6.45575142 5.73326089 9.74287524 
/*Calibration BSpline order: */ 5
/*Calibration BSpline breakpoints: */ 6
/*Number of Control Vals: */41
/*Control Vals: */0. 0.25 0.5 0.75 1. 1.25 1.5 1.75 2. 2.25 2.5 2.75 3. 3.25 3.5 3.75 4. 4.25 4.5 4.75 5. 5.25 5.5 5.75 6. 6.25 6.5 6.75 7. 7.25 7.5 7.75 8. 8.25 8.5 8.75 9. 9.25 9.5 9.75 10. 
/*Number of Result Vals: */41
/*Result Vals: */-0.01515839 -0.01460524 -0.01164586 -0.00619022 0.00184484 0.01219504 0.02472884 0.03889126 0.05456603 0.07165029 0.09008103 0.11038163 0.13251381 0.15661367 0.18245369 0.20862453 0.23478118 0.26102191 0.28807352 0.31526978 0.34210456 0.36813883 0.39405286 0.41741325 0.43729606 0.45240959 0.46319239 0.47097964 0.47738319 0.4830079 0.48713019 0.49064562 0.49369884 0.49599078 0.49830851 0.50075884 0.50231391 0.50351253 0.5042993 0.50538316 0.50658452 
/*Historical Calibration Result:*/
/*Number of Calibration Coefficients: */ 0
/*Calibration Coefficients: */ "!#EMPTY_STRING#!"
/*Calibration BSpline order: */ 0
/*Calibration BSpline breakpoints: */ 0
/*Number of Control Vals: */0
/*Control Vals: */"!#EMPTY_STRING#!"
/*Number of Result Vals: */0
/*Result Vals: */"!#EMPTY_STRING#!"
END_CALIBRATION_MANAGER"""
    variable = CalibrationSection("CALIBRATION_MANAGER",data_chunk)
    a = variable.__str__()
    print(variable)
