#include "PHG4SimpleEventGenerator.h"

#include "PHG4InEvent.h"
#include "PHG4Particle.h"  // for PHG4Particle
#include "PHG4Particlev2.h"
#include "PHG4Utils.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHDataNode.h>      // for PHDataNode
#include <phool/PHNode.h>          // for PHNode
#include <phool/PHNodeIterator.h>  // for PHNodeIterator
#include <phool/PHObject.h>        // for PHObject
#include <phool/getClass.h>
#include <phool/phool.h>  // for PHWHERE

#include <TSystem.h>

#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>  // for gsl_rng_uniform_pos

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>  // for operator<<, endl, basic_ostream
#include <memory>    // for allocator_traits<>::value_type

using namespace std;

PHG4SimpleEventGenerator::PHG4SimpleEventGenerator(const string &name)
  : PHG4ParticleGeneratorBase(name)
{
  return;
}

void PHG4SimpleEventGenerator::add_particles(const std::string &name, const unsigned int num)
{
  _particle_names.push_back(std::make_pair(name, num));
  return;
}

void PHG4SimpleEventGenerator::add_particles(const int pid, const unsigned int num)
{
  _particle_codes.push_back(std::make_pair(pid, num));
  return;
}

void PHG4SimpleEventGenerator::set_eta_range(const double min, const double max)
{
  if (min > max)
  {
    cout << "not setting eta bc etamin " << min << " > etamax: " << max << endl;
    gSystem->Exit(1);
  }
  _eta_min = min;
  _eta_max = max;
  _theta_min = NAN;
  _theta_max = NAN;
  return;
}

void PHG4SimpleEventGenerator::set_theta_range(const double min, const double max)
{
  if (min > max)
  {
    cout << "not setting theta bc thetamin " << min << " > thetamax: " << max << endl;
    gSystem->Exit(1);
  }
  if (min < 0 || max > 2*M_PI)
  {
    std::cout << "min or max outside range (range is 0 to 2pi) min: " << min << ", max: " << max  << std::endl;
    gSystem->Exit(1);
  }
  _theta_min = min;
  _theta_max = max;
  _eta_min = NAN;
  _eta_max = NAN;
  return;
}

void PHG4SimpleEventGenerator::set_phi_range(const double min, const double max)
{
  if (min > max)
  {
    cout << "not setting phi bc phimin " << min << " > phimax: " << max << endl;
    gSystem->Exit(1);
    return;
  }
  if (min < -M_PI || max > M_PI)
  {
    std::cout << "min or max outside range (range is -pi to pi), min: " << min << ", max: " << max << std::endl;
    gSystem->Exit(1);
  }

  _phi_min = min;
  _phi_max = max;
  return;
}

void PHG4SimpleEventGenerator::set_pt_range(const double min, const double max, const double pt_gaus_width)
{
  if (min > max)
  {
    cout << "not setting pt bc ptmin " << min << " > ptmax: " << max << endl;
    return;
  }
  assert(pt_gaus_width >= 0);

  _pt_min = min;
  _pt_max = max;
  _pt_gaus_width = pt_gaus_width;
  _p_min = NAN;
  _p_max = NAN;
  _p_gaus_width = NAN;
  return;
}

void PHG4SimpleEventGenerator::set_p_range(const double min, const double max, const double p_gaus_width)
{
  if (min > max)
  {
    cout << "not setting p bc ptmin " << min << " > ptmax: " << max << endl;
    return;
  }
  assert(p_gaus_width >= 0);
  _pt_min = NAN;
  _pt_max = NAN;
  _pt_gaus_width = NAN;
  _p_min = min;
  _p_max = max;
  _p_gaus_width = p_gaus_width;
  return;
}

void PHG4SimpleEventGenerator::set_vertex_distribution_function(FUNCTION x, FUNCTION y, FUNCTION z)
{
  _vertex_func_x = x;
  _vertex_func_y = y;
  _vertex_func_z = z;
  return;
}

void PHG4SimpleEventGenerator::set_vertex_distribution_mean(const double x, const double y, const double z)
{
  _vertex_x = x;
  _vertex_y = y;
  _vertex_z = z;
  return;
}

void PHG4SimpleEventGenerator::set_vertex_distribution_width(const double x, const double y, const double z)
{
  _vertex_width_x = x;
  _vertex_width_y = y;
  _vertex_width_z = z;
  return;
}

void PHG4SimpleEventGenerator::set_existing_vertex_offset_vector(const double x, const double y, const double z)
{
  _vertex_offset_x = x;
  _vertex_offset_y = y;
  _vertex_offset_z = z;
  return;
}

void PHG4SimpleEventGenerator::set_vertex_size_function(FUNCTION r)
{
  _vertex_size_func_r = r;
  return;
}

void PHG4SimpleEventGenerator::set_vertex_size_parameters(const double mean, const double width)
{
  _vertex_size_mean = mean;
  _vertex_size_width = width;
  return;
}

int PHG4SimpleEventGenerator::InitRun(PHCompositeNode *topNode)
{
  if ((_vertex_func_x != Uniform) && (_vertex_func_x != Gaus))
  {
    cout << PHWHERE << "::Error - unknown vertex distribution function requested" << endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }
  if ((_vertex_func_y != Uniform) && (_vertex_func_y != Gaus))
  {
    cout << PHWHERE << "::Error - unknown vertex distribution function requested" << endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }
  if ((_vertex_func_z != Uniform) && (_vertex_func_z != Gaus))
  {
    cout << PHWHERE << "::Error - unknown vertex distribution function requested" << endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  _ineve = findNode::getClass<PHG4InEvent>(topNode, "PHG4INEVENT");
  if (!_ineve)
  {
    PHNodeIterator iter(topNode);
    PHCompositeNode *dstNode;
    dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));

    _ineve = new PHG4InEvent();
    PHDataNode<PHObject> *newNode = new PHDataNode<PHObject>(_ineve, "PHG4INEVENT", "PHObject");
    dstNode->addNode(newNode);
  }

  if (Verbosity() > 0)
  {
    cout << "================ PHG4SimpleEventGenerator::InitRun() ======================" << endl;
    cout << " Random seed = " << get_seed() << endl;
    cout << " Particles:" << endl;
    for (unsigned int i = 0; i < _particle_codes.size(); ++i)
    {
      cout << "    " << _particle_codes[i].first << ", count = " << _particle_codes[i].second << endl;
    }
    for (unsigned int i = 0; i < _particle_names.size(); ++i)
    {
      cout << "    " << _particle_names[i].first << ", count = " << _particle_names[i].second << endl;
    }
    if (get_reuse_existing_vertex())
    {
      cout << " Vertex Distribution: Set to reuse a previously generated sim vertex" << endl;
      cout << " Vertex offset vector (x,y,z) = (" << _vertex_offset_x << "," << _vertex_offset_y << "," << _vertex_offset_z << ")" << endl;
    }
    else
    {
      cout << " Vertex Distribution Function (x,y,z) = (";
      if (_vertex_func_x == Uniform)
        cout << "Uniform,";
      else if (_vertex_func_x == Gaus)
        cout << "Gaus,";
      if (_vertex_func_y == Uniform)
        cout << "Uniform,";
      else if (_vertex_func_y == Gaus)
        cout << "Gaus,";
      if (_vertex_func_z == Uniform)
        cout << "Uniform";
      else if (_vertex_func_z == Gaus)
        cout << "Gaus";
      cout << ")" << endl;
      cout << " Vertex mean (x,y,z) = (" << _vertex_x << "," << _vertex_y << "," << _vertex_z << ")" << endl;
      cout << " Vertex width (x,y,z) = (" << _vertex_width_x << "," << _vertex_width_y << "," << _vertex_width_z << ")" << endl;
    }
    cout << " Vertex size function (r) = (";
    if (_vertex_size_func_r == Uniform)
      cout << "Uniform";
    else if (_vertex_size_func_r == Gaus)
      cout << "Gaus";
    cout << ")" << endl;
    cout << " Vertex size (mean) = (" << _vertex_size_mean << ")" << endl;
    cout << " Vertex size (width) = (" << _vertex_size_width << ")" << endl;
    if (isfinite(_eta_min) && isfinite(_eta_max))
    {
    cout << " Eta range = " << _eta_min << " - " << _eta_max << endl;
    }
    if (isfinite(_theta_min) && isfinite(_theta_max))
    {
    cout << " Theta range = " << _theta_min << " - " << _theta_max << endl;
    }
    cout << " Phi range = " << _phi_min << " - " << _phi_max << endl;
    if (isfinite(_pt_min) && isfinite(_pt_max))
    {
      cout << " pT range = " << _pt_min << " - " << _pt_max << endl;
    }
    if (isfinite(_p_min) && isfinite(_p_max))
    {
      cout << " p range = " << _p_min << " - " << _p_max << endl;
    }
    cout << " t0 = " << _t0 << endl;
    cout << "===========================================================================" << endl;
  }

  // the definition table should be filled now, so convert codes into names
  for (unsigned int i = 0; i < _particle_codes.size(); ++i)
  {
    int pdgcode = _particle_codes[i].first;
    unsigned int count = _particle_codes[i].second;
    string pdgname = get_pdgname(pdgcode);
    _particle_names.push_back(make_pair(pdgname, count));
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int PHG4SimpleEventGenerator::process_event(PHCompositeNode *topNode)
{
  if (Verbosity() > 0)
  {
    cout << "====================== PHG4SimpleEventGenerator::process_event() =====================" << endl;
    cout << "PHG4SimpleEventGenerator::process_event - reuse_existing_vertex = " << get_reuse_existing_vertex() << endl;
  }

  // vtx_x, vtx_y and vtx_z are doubles from the base class
  // common methods modify those, please no private copies
  // at some point we might rely on them being up to date
  if (!ReuseExistingVertex(topNode))
  {
    // generate a new vertex point
    vtx_x = smearvtx(_vertex_x, _vertex_width_x, _vertex_func_x);
    vtx_y = smearvtx(_vertex_y, _vertex_width_y, _vertex_func_y);
    vtx_z = smearvtx(_vertex_z, _vertex_width_z, _vertex_func_z);
  }

  vtx_x += _vertex_offset_x;
  vtx_y += _vertex_offset_y;
  vtx_z += _vertex_offset_z;

  if (Verbosity() > 0)
  {
    cout << "PHG4SimpleEventGenerator::process_event - vertex center" << get_reuse_existing_vertex()
         << vtx_x << ", " << vtx_y << ", " << vtx_z << " cm"
         << endl;
  }

  int vtxindex = -1;
  int trackid = -1;
  for (unsigned int i = 0; i < _particle_names.size(); ++i)
  {
    string pdgname = _particle_names[i].first;
    int pdgcode = get_pdgcode(pdgname);
    unsigned int nparticles = _particle_names[i].second;

    for (unsigned int j = 0; j < nparticles; ++j)
    {
      if ((_vertex_size_width > 0.0) || (_vertex_size_mean != 0.0))
      {
        double r = smearvtx(_vertex_size_mean, _vertex_size_width, _vertex_size_func_r);

        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        gsl_ran_dir_3d(RandomGenerator(), &x, &y, &z);
        x *= r;
        y *= r;
        z *= r;

        vtxindex = _ineve->AddVtx(vtx_x + x, vtx_y + y, vtx_z + z, _t0);
      }
      else if ((i == 0) && (j == 0))
      {
        vtxindex = _ineve->AddVtx(vtx_x, vtx_y, vtx_z, _t0);
      }

      ++trackid;

      double eta;
      if (!std::isnan(_eta_min) && !std::isnan(_eta_max))
      {
        eta = (_eta_max - _eta_min) * gsl_rng_uniform_pos(RandomGenerator()) + _eta_min;
      }
      else if (!std::isnan(_theta_min) && !std::isnan(_theta_max))
      {
	double theta = (_theta_max - _theta_min) * gsl_rng_uniform_pos(RandomGenerator()) + _theta_min;
        eta = PHG4Utils::get_eta(theta);
      }
      else
      {
        cout << PHWHERE << "Error: neither eta range or theta range was specified" << endl;
	cout << "That should not happen, please inform the software group howthis happened" << std::endl;
        exit(-1);
      }

      double phi = (_phi_max - _phi_min) * gsl_rng_uniform_pos(RandomGenerator()) + _phi_min;

      double pt;
      if (!std::isnan(_p_min) && !std::isnan(_p_max) && !std::isnan(_p_gaus_width))
      {
        pt = ((_p_max - _p_min) * gsl_rng_uniform_pos(RandomGenerator()) + _p_min + gsl_ran_gaussian(RandomGenerator(), _p_gaus_width)) / cosh(eta);
      }
      else if (!std::isnan(_pt_min) && !std::isnan(_pt_max) && !std::isnan(_pt_gaus_width))
      {
        pt = (_pt_max - _pt_min) * gsl_rng_uniform_pos(RandomGenerator()) + _pt_min + gsl_ran_gaussian(RandomGenerator(), _pt_gaus_width);
      }
      else
      {
        cout << PHWHERE << "Error: neither a p range or pt range was specified" << endl;
        exit(-1);
      }

      double px = pt * cos(phi);
      double py = pt * sin(phi);
      double pz = pt * sinh(eta);
      double m = get_mass(pdgcode);
      double e = sqrt(px * px + py * py + pz * pz + m * m);

      PHG4Particle *particle = new PHG4Particlev2();
      particle->set_track_id(trackid);
      particle->set_vtx_id(vtxindex);
      particle->set_parent_id(0);
      particle->set_name(pdgname);
      particle->set_pid(pdgcode);
      particle->set_px(px);
      particle->set_py(py);
      particle->set_pz(pz);
      particle->set_e(e);

      _ineve->AddParticle(vtxindex, particle);
      if (EmbedFlag() != 0) _ineve->AddEmbeddedParticle(particle, EmbedFlag());
    }
  }

  if (Verbosity() > 0)
  {
    _ineve->identify();
    cout << "======================================================================================" << endl;
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

double
PHG4SimpleEventGenerator::smearvtx(const double position, const double width, FUNCTION dist) const
{
  double res = position;
  if (dist == Uniform)
  {
    res = (position - width) + 2 * gsl_rng_uniform_pos(RandomGenerator()) * width;
  }
  else if (dist == Gaus)
  {
    res = position + gsl_ran_gaussian(RandomGenerator(), width);
  }
  return res;
}
