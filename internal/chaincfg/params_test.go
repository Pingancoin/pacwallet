package chaincfg_test

import (
	"testing"

	"github.com/Pingancoin/pacwallet/internal/chaincfg"
)

func TestStageNetParamsAreDistinct(t *testing.T) {
	mainnet := chaincfg.MainNetParams()
	stagenet := chaincfg.StageNetParams()
	if stagenet.Name != "stagenet" {
		t.Fatalf("stagenet name = %q", stagenet.Name)
	}
	if stagenet.DefaultPort == mainnet.DefaultPort || stagenet.AddressPrefix == mainnet.AddressPrefix {
		t.Fatalf("stagenet is not distinct from mainnet: %+v", stagenet)
	}
}
