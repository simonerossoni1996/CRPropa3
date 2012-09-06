#include "mpc/module/ElectronPairProduction.h"

#include <fstream>
#include <limits>
#include <stdexcept>

namespace mpc {

ElectronPairProduction::ElectronPairProduction(int photonField) {
	init(photonField);
}

void ElectronPairProduction::init(int photonField) {
	this->photonField = photonField;
	switch (photonField) {
	case CMB:
		setDescription("ElectronPairProduction:CMB");
		init(getDataPath("ElectronPairProduction/cmb.txt"));
		break;
	case IRB:
		setDescription("ElectronPairProduction:IRB");
		init(getDataPath("ElectronPairProduction/ir.txt"));
		break;
	case CMB_IRB:
		setDescription("ElectronPairProduction:CMB_IRB");
		init(getDataPath("ElectronPairProduction/cmbir.txt"));
		break;
	default:
		throw std::runtime_error(
				"mpc::ElectronPairProduction: unknown photon background");
	}
}

void ElectronPairProduction::init(std::string filename) {
	// load energy loss rate table
	std::ifstream infile(filename.c_str());

	if (!infile.good())
		throw std::runtime_error(
				"mpc::ElectronPairProduction: could not open file " + filename);

	std::vector<double> x, y;
	while (infile.good()) {
		if (infile.peek() != '#') {
			double a, b;
			infile >> a >> b;
			if (infile) {
				energy.push_back(a * eV);
				lossRate.push_back(b * eV / Mpc);
			}
		}
		infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	infile.close();
}

void ElectronPairProduction::process(Candidate *candidate) const {
	double A = candidate->current.getMassNumber();
	double E = candidate->current.getEnergy();
	double z = candidate->getRedshift();

	double EpA = E / A * (1 + z);
	if (EpA < energy.front())
		return;

	double rate;
	if (EpA < energy.back())
		rate = interpolate(EpA, &energy[0], &lossRate[0]);
	else
		rate = lossRate.back() * pow(EpA / energy.back(), 0.4); // extrapolation

	double step = candidate->getCurrentStep() / (1 + z);
	double Z = candidate->current.getChargeNumber();

	// dE(E) = Z^2 * loss_rate(E/A) * step
	double dE = Z * Z * rate * pow(1 + z, 3) * step;
	candidate->current.setEnergy(E - dE);
}

} // namespace mpc
