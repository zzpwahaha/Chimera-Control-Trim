import sys
# Constants for directory paths
LIBRARY_DIR = "C:\\Chimera\\B240_data_analysis\\Library\\"
CHIMERA_GENTOOL_DIR = LIBRARY_DIR + "ChimeraGenTools\\"
FITTER_DIR = CHIMERA_GENTOOL_DIR + "fitters"

sys.path.append(CHIMERA_GENTOOL_DIR)
sys.path.append(FITTER_DIR)

from typing import List
import numpy as np
import ExpFile as exp
import AnalysisHelpers as ah
import MatplotlibPlotters as mp
from fitters import decaying_cos
from fitters.Gaussian import gaussian
from fitters.Polynomial import Quadratic
from remoteDataPaths import get_data_files

class DataAnalysis:
    def __init__(self, year, month, day, data_name, 
                 window, thresholds, binnings, maximaLocs=None,
                 annotate_title="", annotate_note="",
                 save_cache=False, read_cache=False, cache_path=None):
        self.year = year
        self.month = month
        self.day = day
        self.data_name = data_name
        self.annotate_title = annotate_title
        self.annotate_note = annotate_note
        self.save_cache = save_cache
        self.read_cache = read_cache
        self.cache_path = cache_path
        self.data_path = None
        self.remote_data_folder = None
        self.cached_data_folder = None
        self.exp_file = None
        self.andor_datas = None

        # Instance variables for analysis parameters, with defaults
        self.window = window
        self.thresholds = thresholds
        self.binnings = binnings
        self.maximaLocs = maximaLocs
        
        self._initialize_data()

    def _initialize_data(self):
        datas, remote_data_folder, cached_data_folder = get_data_files(
            year=self.year, month=self.month, day=self.day, data_runs=self.data_name, 
            save_cache=self.save_cache, read_cache=self.read_cache, cache_path=self.cache_path
        )
        self.data_path = datas
        self.remote_data_folder = remote_data_folder
        self.cached_data_folder = cached_data_folder

        if not exp.checkAnnotation(self.data_name, data_address=self.data_path, force=False):
            with exp.ExpFile(data_address=self.data_path) as file:
                file.open_hdf5(self.data_name, openFlag='a', useBase=True)
                exp._annotate(file, self.annotate_title, self.annotate_note)

        CURRENT_VERSION = 1.01
        self.exp_file = exp.ExpFile(file_id=self.data_name, data_address=self.data_path, expFile_version=CURRENT_VERSION)
        print(exp.getAnnotation(fid=self.data_name, data_address=self.data_path))
        self.andor_datas = self.exp_file.get_pics(image_num_per_rep=self.exp_file.pics_per_rep)

        if self.maximaLocs is None:
            self.maximaLocs = self.findAtomLocs(self.thresholds)

    def findAtomLocs(self, image_threshold):
        maximaLocs_MP = ah.findAtomLocs(
            pic=ah.softwareBinning(None, self.andor_datas[0].mean(axis=(0, 1))),
            n_cluster_row=1,
            neighborhood_size=11,
            threshold=0,
            window=self.window,
            sort='MatchArray',
            debug_plot=False,
            advanced_option=dict({"active": True, "image_threshold": image_threshold, "score_threshold": 10}),
            multi_points_option=dict({"active": True, "search_square": 3, "num_points": 3})
        )
        return maximaLocs_MP

    def get_survival_result_1D(self):
        result = ah.getAtomSurvivalData(
            self.andor_datas,
            atomLocation=self.maximaLocs,
            window=self.window,
            bins=self.binnings,
            thresholds=self.thresholds
        )
        x, y, yerr = self.exp_file.individual_keys[0][:], result['survival_mean'][:], result['survival_err'][:]
        return x,y,yerr

    def get_loading_result_1D(self):
        result = ah.getAtomSurvivalData(
            self.andor_datas,
            atomLocation=self.maximaLocs,
            window=self.window,
            bins=self.binnings,
            thresholds=self.thresholds
        )
        x, y, yerr = self.exp_file.individual_keys[0][:], result['loading_rate'][:], result['loading_rate_err'][:]
        return x,y,yerr

    def analyze_mako_data(self, mako_idx: int, function = gaussian, debug=False) -> List[ah.unc.UFloat]:
        if mako_idx not in [1,2,3,4]:
            raise ValueError("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
        try:
            if mako_idx==1:
                mpic = self.exp_file.get_mako1_pics()[0]
            elif mako_idx==2:
                mpic = self.exp_file.get_mako2_pics()[0]
            elif mako_idx==3:
                mpic = self.exp_file.get_mako3_pics()[0]
            else:
                mpic = self.exp_file.get_mako4_pics()[0]
        except KeyError as err:
            raise err

        positions = []
        for idv, mvar in enumerate(mpic):
            tmp = []
            for idr, mrep in enumerate(mvar):
                hori, vert = mrep.mean(axis=0), mrep.mean(axis=1)
                _1 = ah.fit_data(np.arange(hori.size), hori, fit_function=function, use_unc=False)[0][1]
                _2 = ah.fit_data(np.arange(vert.size), vert, fit_function=function, use_unc=False)[0][1]
                tmp.append([_1,_2])
            positions.append(tmp)
        positions = np.array(positions)
        avg_pos = positions.mean(axis=1) # average over repetitions
        horizontal, vertical = avg_pos[:,0], avg_pos[:,1] # shape as [var]

        if debug:
            vertical = positions[:,:,1].flatten() # positions.mean(axis=1)[:,1] # [:,0] for horizontal, [:,1] for vertical
            fig, ax = mp._plotDiscetePointsAndDistribution(
                ah.unp.nominal_values(vertical),
                "vertical pixel",
                plot_y_err=ah.unp.std_devs(vertical),
                exp_file=self.exp_file, fit=False
            )
            ax[0].set_xlabel('exp rep #')

            vertical = positions[:,:,0].flatten() # positions.mean(axis=1)[:,1] # [:,0] for horizontal, [:,1] for vertical
            fig, ax = mp._plotDiscetePointsAndDistribution(
                ah.unp.nominal_values(vertical),
                "horizontal pixel",
                plot_y_err=ah.unp.std_devs(vertical),
                exp_file=self.exp_file, fit=False
            )
            ax[0].set_xlabel('exp rep #')

        return vertical, horizontal

    def analyze_data(self, xkey = None, function = gaussian, p0=None, debug=False) -> List[ah.unc.UFloat]:
        result = ah.getAtomSurvivalData(
            self.andor_datas,
            atomLocation=self.maximaLocs,
            window=self.window,
            bins=self.binnings,
            thresholds=self.thresholds
        )

        x, y, yerr = self.exp_file.individual_keys[0][:], result['survival_mean'][:], result['survival_err'][:]
        if xkey is not None:
            xkey = np.array(xkey)
            if xkey.shape != y.shape:
                raise ValueError("xkey shape does not match y!")
            x = xkey.copy()
        x_fit = x[yerr!=0]; y_fit = y[yerr!=0]; yerr_fit = yerr[yerr!=0]
        if p0 is None:
            p0 = function.guess(x, y)
        p, c = ah.fit(function.f, x_fit, y_fit, sigma=yerr_fit, p0=p0)
        punc = ah.getConfidentialInterval(p, c, n_sample=x.size)
        print(ah.printFittingResult(func=function, popt_unc=punc)[1])

        if debug:
            guess = None
            fig, ax = mp._plotStandard1D(
                x, y, yerr, exp_file=self.exp_file, fitb=True, fit_function=function, guess=guess, 
                ignore_zero_unc=True, use_unc=True, plot_guess=False
            )
            ax.set_xlabel(self.exp_file.key_name[0])
            ax.set_ylabel('survival')
            mp.plt.show()

        return punc

    def analyze_data_2D(self, xkey0 = None, function_d0 = Quadratic, function_d1 = gaussian, debug=False) -> List[ah.unc.UFloat]:
        result = ah.getAtomSurvivalData(
            self.andor_datas,
            atomLocation=self.maximaLocs,
            window=self.window,
            bins=self.binnings,
            thresholds=self.thresholds
        )

        reshape_kw = {'switch2DAxis':False, 'scanAxisOrder':None} # Dim 0 -> bias_e_x, Dim 1 -> resonance_scan
        _,plot_x, plot_c,_,_ = mp._generatePlotX(exp_file=self.exp_file, **reshape_kw)
        x_axis = plot_x

        res0_survival = mp._reshapeForMultiDimensionResult(data=np.array([result['survival_mean']]), exp_file=self.exp_file, **reshape_kw)[0]
        res0_survival_std = mp._reshapeForMultiDimensionResult(data=np.array([result['survival_err']]), exp_file=self.exp_file, **reshape_kw)[0]
        res0_survival_std[res0_survival_std==0]=res0_survival_std[res0_survival_std!=0].min()

        y_traces = res0_survival
        uncertainties = res0_survival_std

        popt_uncs = []
        for idx, (x,y,yerr) in enumerate(zip(x_axis, y_traces, uncertainties)):
            function = function_d1
            p0=function.guess(x,y)
            p,c = ah.fit(function.f, x[:], y[:], p0=p0,sigma = yerr[:],) 
            punc = ah.getConfidentialInterval(p,c,n_sample=x.size)
            # print(ah.printFittingResult(func=function, popt_unc=punc)[1])
            popt_uncs.append(punc)
        popt_uncs=np.array(popt_uncs)

        x,y,yerr = self.exp_file.individual_keys[0][:], ah.nominal(popt_uncs[:,1])[:] , ah.std_dev(popt_uncs[:,1])[:] 
        if xkey0 is not None:
            xkey0 = np.array(xkey0)
            if xkey0.shape != y.shape:
                raise ValueError("xkey shape does not match y!")
            x = xkey0
        function = function_d0
        p0 =  function.guess(x,y)
        p,c = ah.fit(function.f, x[:], y[:], p0=p0, sigma=yerr[:])
        punc = ah.getConfidentialInterval(p,c,n_sample=x.size)
        print(ah.printFittingResult(func=function, popt_unc=punc)[1])

        if debug:
            guess = None
            fig, ax = mp._plotStandard1D(
                x, y, yerr, exp_file=self.exp_file, fitb=True, fit_function=function_d0, guess=guess, 
                ignore_zero_unc=True, use_unc=True, plot_guess=False
            )
            ax.set_xlabel(self.exp_file.key_name[0])
            ax.set_ylabel('survival')
            ax.autoscale()
            ax.relim()
            mp.plt.show()

        return punc

# Usage example
if __name__ == "__main__":
    window = [0, 0, 200, 30]
    thresholds = 60
    binnings = np.linspace(0, 240, 241)
    analysis_locs = DataAnalysis(year='2024', month='May', day='15', data_name='data_1', 
                                 window=window, thresholds=104, binnings=binnings)
    analysis = DataAnalysis(year='2024', month='August', day='8', data_name='data_1', 
                            maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings)
    analysis_2D = DataAnalysis(year='2024', month='August', day='6', data_name='data_12', 
                            maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings)

    # analysis.analyze_data(debug=True)
    result = analysis_2D.analyze_data_2D(debug=True)
    print(result[1].n)
    print(f"Optimal field for X is {result[1]:.2S} ")

