#ifndef G4INTT_INTTDEADMAP_H
#define G4INTT_INTTDEADMAP_H

#include <g4detectors/PHG4CellDefs.h>

#include <phool/PHObject.h>
#include <set>

class INTTDeadMap : public PHObject
{
 public:
  typedef std::set<PHG4CellDefs::keytype> Map;

  virtual ~INTTDeadMap() {}
  virtual void Reset() {}
  virtual int isValid() const;

  virtual void identify(std::ostream &os = std::cout) const;

  void addDeadChannelINTT(const int layer,
                          const int ladder_phi, const int ladder_z,
                          const int strip_z, const int strip_phi);
  virtual void addDeadChannel(PHG4CellDefs::keytype key) {return;}

  virtual bool isDeadChannel(PHG4CellDefs::keytype key) const {return false;}
  bool isDeadChannelINTT(const int layer,
                         const int ladder_phi, const int ladder_z,
                         const int strip_z, const int strip_phi) const;

  //! return all towers
  virtual const Map &getDeadChannels(void) const;
  virtual Map &getDeadChannels(void);

  virtual unsigned int size() const { return 0; }

  static PHG4CellDefs::keytype getINTTKey(int layer,
                                          int ladder_phi, int ladder_z,
                                          int strip_z, int strip_phi);

  static int getWildCardID() { return s_wildCardID; }

 protected:
  INTTDeadMap()
  {
  }

 private:
  static int s_wildCardID;

  ClassDef(INTTDeadMap, 1)
};

#endif /* G4INTT_INTTDEADMAP_H */
